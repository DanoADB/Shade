// Shade browser graphics probe. Original code, isolated from SDL3 GPU until the renderer port.
// Build with Emscripten (em++) and serve through web/serve.py.
#include <GLES3/gl3.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <string>
#include <limits>

namespace {
GLuint program = 0, vao = 0, vertices = 0;
GLint position = -1, color = -1, transform = -1;
float angle = 0.45f, zoom = 9.0f, pitch = 0.65f;
bool held[512] {};
double previous = 0.0;
GLsizei vertexCount = 0;


const char* vertexSource = R"(#version 300 es
precision highp float;
in vec3 aPosition;
in vec3 aColor;
uniform mat4 uTransform;
out vec3 vColor;
void main() {
    gl_Position = uTransform * vec4(aPosition, 1.0);
    vColor = aColor;
})";
const char* fragmentSource = R"(#version 300 es
precision mediump float;
in vec3 vColor;
out vec4 fragColor;
void main() { fragColor = vec4(vColor, 1.0); })";

GLuint compile(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char message[2048] {};
        glGetShaderInfoLog(shader, sizeof(message), nullptr, message);
        std::fprintf(stderr, "Shader: %s\n", message);
        std::exit(1);
    }
    return shader;
}
struct V { float x,y,z,r,g,b; };
float elevation(float x,float z) {
    const float radius = std::sqrt(x*x+z*z);
    const float shore = std::fmax(0.0f, 1.0f - std::pow(radius/4.65f, 2.0f));
    return shore*(0.65f + 0.38f*std::sin(x*1.8f)*std::cos(z*1.4f)
                   + 0.23f*std::sin(z*2.9f+x*0.7f))-0.16f;
}
void push(std::vector<V>& data,float x,float z) {
    const float y=elevation(x,z);
    const float height=std::fmax(0.f,y);
    data.push_back({x,y,z,0.16f+0.28f*height,0.37f+0.22f*height,0.22f+0.12f*height});
}
void upload(const std::vector<V>& data) {
    if (data.empty()) return;
    if (!vao) glGenVertexArrays(1,&vao);
    glBindVertexArray(vao);
    if (!vertices) glGenBuffers(1,&vertices);
    glBindBuffer(GL_ARRAY_BUFFER,vertices);
    glBufferData(GL_ARRAY_BUFFER,data.size()*sizeof(V),data.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(position);
    glVertexAttribPointer(position,3,GL_FLOAT,GL_FALSE,sizeof(V),(void*)0);
    glEnableVertexAttribArray(color);
    glVertexAttribPointer(color,3,GL_FLOAT,GL_FALSE,sizeof(V),(void*)(3*sizeof(float)));
    vertexCount=static_cast<GLsizei>(data.size());
}
void geometry() {
    std::vector<V> data;
    constexpr int grid=96;
    data.reserve(grid*grid*6);
    for(int z=0;z<grid;z++) for(int x=0;x<grid;x++) {
        float x0=(x/(float)grid-0.5f)*10.f, x1=((x+1)/(float)grid-0.5f)*10.f;
        float z0=(z/(float)grid-0.5f)*10.f, z1=((z+1)/(float)grid-0.5f)*10.f;
        push(data,x0,z0);push(data,x1,z0);push(data,x0,z1);
        push(data,x0,z1);push(data,x1,z0);push(data,x1,z1);
    }
    upload(data);
}
float matrix[16];
void render() {
    double now=emscripten_get_now();
    float dt=(float)std::fmin((now-previous)/1000.,0.05);
    previous=now;
    if(held['a']||held['A']||held[37]) angle-=dt*1.1f;
    if(held['d']||held['D']||held[39]) angle+=dt*1.1f;
    if(held['w']||held['W']||held[38]) pitch=std::fmin(1.25f,pitch+dt*0.8f);
    if(held['s']||held['S']||held[40]) pitch=std::fmax(0.16f,pitch-dt*0.8f);
    double cssWidth=0,cssHeight=0;
    emscripten_get_element_css_size("#canvas", &cssWidth, &cssHeight);
    const double ratio=emscripten_get_device_pixel_ratio();
    const int desiredWidth=static_cast<int>(std::fmax(1.0,cssWidth*ratio));
    const int desiredHeight=static_cast<int>(std::fmax(1.0,cssHeight*ratio));
    int currentWidth=0,currentHeight=0;
    emscripten_get_canvas_element_size("#canvas",&currentWidth,&currentHeight);
    if(currentWidth!=desiredWidth || currentHeight!=desiredHeight)
        emscripten_set_canvas_element_size("#canvas",desiredWidth,desiredHeight);
    int width=0,height=0;
    emscripten_get_canvas_element_size("#canvas",&width,&height);
    glViewport(0,0,width,height);
    glClearColor(0.09f,0.20f,0.27f,1.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    const float aspect=width/(float)std::fmax(1,height);
    const float f=1.9f;
    const float c=std::cos(angle),s=std::sin(angle),cp=std::cos(pitch),sp=std::sin(pitch);
    // Column-major projection * view. Ground plane looking down toward island center.
    const float projection[16]={f/aspect,0,0,0, 0,f,0,0, 0,0,-1.002f,-1.f, 0,0,-0.2002f,0};
    const float view[16]={c*1.f,-s*sp,-s*cp,0, 0,cp,-sp,0, s*1.f,c*sp,c*cp,0, 0,-0.5f,-zoom,1};
    for(int col=0;col<4;col++)for(int row=0;row<4;row++){
        float v=0;for(int k=0;k<4;k++)v+=projection[k*4+row]*view[col*4+k];
        matrix[col*4+row]=v;
    }
    glUseProgram(program);
    glUniformMatrix4fv(transform,1,GL_FALSE,matrix);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES,0,vertexCount);
}
EM_BOOL key(int type,const EmscriptenKeyboardEvent* event,void*) {
    const int code=(int)event->keyCode;
    if(code>=0&&code<512)held[code]=(type==EMSCRIPTEN_EVENT_KEYDOWN);
    return code==37||code==38||code==39||code==40;
}
EM_BOOL blur(int,const EmscriptenFocusEvent*,void*) {
    for(bool& pressed:held) pressed=false;
    return EM_FALSE;
}
EM_BOOL wheel(int,const EmscriptenWheelEvent* event,void*) {
    zoom=std::fmax(5.0f,std::fmin(18.0f,zoom+(float)event->deltaY*0.01f));
    return EM_TRUE;
}
}
// Called from the file input. Each triangle is three lines of XYZ RGB vertices.
// This mesh is derived locally from authorized data and is never uploaded.
extern "C" EMSCRIPTEN_KEEPALIVE int shade_load_mesh(const char* text, int length) {
    if (!text || length <= 0 || length > 100000000) return -1;
    std::istringstream input(std::string(text,static_cast<size_t>(length)));
    std::string marker;
    if (!(input >> marker) || marker != "SHADE_MESH_V1") return -2;
    std::vector<V> data;
    V v{};
    float minx=std::numeric_limits<float>::max(), maxx=-minx;
    float minz=minx,maxz=-minx,miny=minx,maxy=-minx;
    while(input >> v.x >> v.y >> v.z >> v.r >> v.g >> v.b) {
        if (!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||
            !std::isfinite(v.r)||!std::isfinite(v.g)||!std::isfinite(v.b)) return -3;
        minx=std::min(minx,v.x);maxx=std::max(maxx,v.x);
        miny=std::min(miny,v.y);maxy=std::max(maxy,v.y);
        minz=std::min(minz,v.z);maxz=std::max(maxz,v.z);
        data.push_back(v);
        if(data.size()>1800000) return -4;
    }
    if(data.empty() || data.size()%3!=0 || !input.eof()) return -5;
    const float scale=10.f/std::max(0.001f,std::max(maxx-minx,maxz-minz));
    for(auto& p:data) {
        p.x=(p.x-(minx+maxx)*0.5f)*scale;
        p.y=(p.y-(miny+maxy)*0.5f)*scale;
        p.z=(p.z-(minz+maxz)*0.5f)*scale;
    }
    upload(data);
    zoom=12.f;
    return static_cast<int>(data.size()/3);
}
int main() {
    EmscriptenWebGLContextAttributes attributes;
    emscripten_webgl_init_context_attributes(&attributes);
    attributes.majorVersion=2;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context=emscripten_webgl_create_context("#canvas",&attributes);
    if(context<=0) {std::fprintf(stderr,"WebGL2 unavailable: %lu\n",static_cast<unsigned long>(context));return 1;}
    emscripten_webgl_make_context_current(context);
    GLuint vs=compile(GL_VERTEX_SHADER,vertexSource),fs=compile(GL_FRAGMENT_SHADER,fragmentSource);
    program=glCreateProgram();glAttachShader(program,vs);glAttachShader(program,fs);glLinkProgram(program);
    GLint linked=0;glGetProgramiv(program,GL_LINK_STATUS,&linked);
    if(!linked){std::fprintf(stderr,"Program link failed\n");return 1;}
    glDeleteShader(vs);glDeleteShader(fs);
    position=glGetAttribLocation(program,"aPosition");
    color=glGetAttribLocation(program,"aColor");
    transform=glGetUniformLocation(program,"uTransform");
    geometry();
    glEnable(GL_DEPTH_TEST);
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,nullptr,true,key);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,nullptr,true,key);
    emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,nullptr,true,blur);
    emscripten_set_wheel_callback("#canvas",nullptr,true,wheel);
    previous=emscripten_get_now();
    render();
    EM_ASM({window.shadeProbeReady();});
    emscripten_set_main_loop(render,0,true);
    return 0;
}
