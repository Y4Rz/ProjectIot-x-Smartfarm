<?php
namespace App\Services;
use PhpMqtt\Client\MqttClient;
use PhpMqtt\Client\ConnectionSettings;
class MqttPublisher {
 public function publishControl(array $payload): void {
  $host=(string)config('services.mqtt.host'); $port=(int)config('services.mqtt.port');
  $client=new MqttClient($host,$port,'web-'.bin2hex(random_bytes(5)));
  $settings=(new ConnectionSettings)->setConnectTimeout(5)->setUseTls(false);
  $user=config('services.mqtt.username'); $pass=config('services.mqtt.password');
  if($user) $settings=$settings->setUsername($user)->setPassword((string)$pass);
  $client->connect($settings,true);
  $client->publish((string)config('services.mqtt.control_topic'),json_encode($payload,JSON_UNESCAPED_SLASHES), (int)config('services.mqtt.qos'), false);
  $client->disconnect();
 }
}
