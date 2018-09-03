
FROM rabbitmq

RUN rabbitmq-plugins enable --offline \
    rabbitmq_management \
    rabbitmq_mqtt \
    rabbitmq_web_mqtt \
    rabbitmq_shovel \
    rabbitmq_shovel_management

# Fix nodename
#RUN echo 'NODENAME=rabbit@localhost' > /etc/rabbitmq/rabbitmq-env.conf

EXPOSE 15671 15672 1883 8883 5672
