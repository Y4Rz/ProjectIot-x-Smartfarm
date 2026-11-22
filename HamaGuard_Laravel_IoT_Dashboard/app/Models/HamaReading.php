<?php
namespace App\Models;
use Illuminate\Database\Eloquent\Model;
class HamaReading extends Model {
 protected $fillable=['gerakan','jarak_cm','status','alarm','ir','buzzer_ok','lonceng','mode','payload','received_at'];
 protected $casts=['gerakan'=>'boolean','jarak_cm'=>'float','buzzer_ok'=>'boolean','payload'=>'array','received_at'=>'datetime'];
}
