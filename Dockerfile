FROM python:3.9-slim-buster

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update

COPY ./requirements.txt .
RUN pip install -U pip \
    && pip install -r requirements.txt

ADD ./test /test

CMD ["pytest", "/test"]
