<?php
namespace App\Http\Controllers;
use Illuminate\Http\Request;
use App\Models\HamaReading;
use App\Services\MqttPublisher;
class DashboardController extends Controller {
 public function index(){return view('dashboard');}
 public function latest(){
  $latest=HamaReading::latest('received_at')->first();
  $history=HamaReading::latest('received_at')->limit(20)->get()->reverse()->values();
  return response()->json(['latest'=>$latest,'history'=>$history,'total'=>HamaReading::count(),'danger'=>HamaReading::where('status','BAHAYA')->count()]);
 }
 public function control(Request $request,MqttPublisher $mqtt){
  $data=$request->validate(['mode'=>'nullable|in:AUTO,MANUAL','buzzer'=>'nullable|in:ON,OFF','led'=>'nullable|in:ON,OFF','ir'=>'nullable|in:ON,OFF','lonceng'=>'nullable|in:ON,OFF']);
  if(empty($data)) return response()->json(['message'=>'Tidak ada perintah.'],422);
  if(($data['mode']??null)==='AUTO') $payload=['mode'=>'AUTO'];
  else $payload=array_merge(['mode'=>'MANUAL'],$data);
  try{$mqtt->publishControl($payload);return response()->json(['message'=>'Perintah berhasil dikirim ke broker MQTT.','payload'=>$payload]);}
  catch(\Throwable $e){report($e);return response()->json(['message'=>'Gagal mengirim MQTT: '.$e->getMessage()],502);}
 }
}
