<?php
namespace App\Console\Commands;
use Illuminate\Console\Command;
use Illuminate\Support\Facades\Log;
use App\Models\HamaReading;
use PhpMqtt\Client\MqttClient;
use PhpMqtt\Client\ConnectionSettings;
class ListenMqtt extends Command {
 protected $signature='mqtt:listen'; protected $description='Subscribe status ESP32 dari broker MQTT dan simpan ke database';
 public function handle(): int {
  $host=(string)config('services.mqtt.host'); $port=(int)config('services.mqtt.port');
  $client=new MqttClient($host,$port,'laravel-sub-'.bin2hex(random_bytes(4)));
  $settings=(new ConnectionSettings)->setConnectTimeout(10)->setUseTls(false)->setKeepAliveInterval(60);
  $user=config('services.mqtt.username'); if($user) $settings=$settings->setUsername($user)->setPassword((string)config('services.mqtt.password'));
  $this->info("Menghubungkan ke {$host}:{$port} ...");
  try {
   $client->connect($settings,true); $this->info('Terhubung. Menunggu pesan status; tekan Ctrl+C untuk berhenti.');
   $client->subscribe((string)config('services.mqtt.status_topic'), function(string $topic,string $message){
    $data=json_decode($message,true); if(!is_array($data)){Log::warning('Payload MQTT bukan JSON valid',['message'=>$message]);return;}
    HamaReading::create([
     'gerakan'=>(bool)($data['gerakan']??false),'jarak_cm'=>is_numeric($data['jarak_cm']??null)?$data['jarak_cm']:null,
     'status'=>strtoupper((string)($data['status']??'AMAN')),'alarm'=>$data['alarm']??null,'ir'=>$data['ir']??null,
     'buzzer_ok'=>array_key_exists('buzzer_ok',$data)?(bool)$data['buzzer_ok']:null,'lonceng'=>$data['lonceng']??null,
     'mode'=>$data['mode']??null,'payload'=>$data,'received_at'=>now()
    ]);
    $this->line(now()->format('H:i:s').' | '.($data['status']??'UNKNOWN').' | jarak: '.($data['jarak_cm']??'null').' cm');
   },(int)config('services.mqtt.qos'));
   $client->loop(true);
  } catch(\Throwable $e){$this->error('MQTT error: '.$e->getMessage());Log::error('MQTT listener failed',['exception'=>$e]);return self::FAILURE;}
  finally {try{$client->disconnect();}catch(\Throwable $e){}}
  return self::SUCCESS;
 }
}
