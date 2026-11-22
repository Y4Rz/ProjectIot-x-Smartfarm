<?php
use Illuminate\Support\Facades\Route;
use App\Http\Controllers\DashboardController;
Route::get('/',[DashboardController::class,'index'])->name('dashboard');
Route::get('/api/latest',[DashboardController::class,'latest'])->name('api.latest');
Route::post('/api/control',[DashboardController::class,'control'])->name('api.control');
