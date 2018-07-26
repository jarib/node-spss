FROM node:10

RUN apt-get update
RUN apt-get install build-essential unzip

WORKDIR /app

ADD package.json .
ADD yarn.lock .

RUN yarn spssio:fetch
RUN yarn spssio:unpack

COPY . /app

RUN yarn spssio:fix

CMD ["bash"]

# RUN yarn install
# RUN yarn test