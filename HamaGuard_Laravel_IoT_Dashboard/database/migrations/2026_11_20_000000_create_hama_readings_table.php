<?php
use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;
return new class extends Migration { public function up(): void { Schema::create('hama_readings', function(Blueprint $table){$table->id();$table->boolean('gerakan')->default(false);$table->decimal('jarak_cm',8,2)->nullable();$table->string('status',20)->default('AMAN');$table->string('alarm',10)->nullable();$table->string('ir',10)->nullable();$table->boolean('buzzer_ok')->nullable();$table->string('lonceng',10)->nullable();$table->string('mode',10)->nullable();$table->json('payload')->nullable();$table->timestamp('received_at')->useCurrent();$table->timestamps();$table->index(['status','received_at']);});} public function down(): void {Schema::dropIfExists('hama_readings');}};
