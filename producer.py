#!/usr/bin/env python
import pika

connection = pika.BlockingConnection(
    pika.ConnectionParameters(host='rabbitmq'))
channel = connection.channel()

#channel.queue_declare(queue='test', durable=True)

while True:
    channel.basic_publish(exchange='',
                            routing_key='test',
                            body="Subject: Hello\r\n\r\nHello!",
                            properties=pika.BasicProperties(
                                headers={
                                    "sender": "root@example.com",
                                    "recipients": [
                                        "erik@halon.io"
                                    ],
                                    "metadata": "{\"json\":[1,2,3]}",
                                }
                            )
                        )
    break

connection.close()