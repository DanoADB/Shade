FROM python:3.12-alpine
WORKDIR /app
COPY web/serve.py /app/web/serve.py
COPY web/dist/ /app/web/dist/
ENV PYTHONUNBUFFERED=1
EXPOSE 8080
CMD ["python", "/app/web/serve.py"]
