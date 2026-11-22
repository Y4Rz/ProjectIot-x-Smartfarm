<?php
return ['mqtt'=>['host'=>env('MQTT_HOST','broker.hivemq.com'),'port'=>env('MQTT_PORT',1883),'username'=>env('MQTT_USERNAME'),'password'=>env('MQTT_PASSWORD'),'status_topic'=>env('MQTT_STATUS_TOPIC','pertanian/hama/status'),'control_topic'=>env('MQTT_CONTROL_TOPIC','pertanian/hama/kontrol'),'qos'=>env('MQTT_QOS',0)]];
