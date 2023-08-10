# RabbitMQ consumer plugin

This plugin allows you to consume messages from RabbitMQ. See [producer.py](producer.py) for sample

## Installation

Follow the [instructions](https://docs.halon.io/manual/comp_install.html#installation) in our manual to add our package repository and then run the below command.

### Ubuntu

```
apt-get install halon-extras-rabbitmq-consumer
```

### RHEL

```
yum install halon-extras-rabbitmq-consumer
```

## Configuration


For the configuration schema, see [rabbitmq-consumer.schema.json](rabbitmq-consumer.schema.json). Below is a sample configuration.

> **_Important!_**
>
> This plugin requires a server configuration with the id `rabbitmq` and type `plugin`.
> ```
> servers:
>  - id: rabbitmq
>    type: plugin
> ```

### smtpd.yaml

```
plugins:
  - id: rabbitmq-consumer
    config:
      queues:
       - id: test
         queue: test
         hostname: rabbitmq
         threads: 32
```