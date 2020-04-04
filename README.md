# fluid simulation in directx11
パーティクルで流体の運動をシミュレーションしています。流体の計算式は、SPH(Smoothed-particle hydrodynamic)に基づいて、コードを実装しました。  
use sph and pbf with GPGPU in directX11 and research possible way to use it in games.  
## 制作環境
Window10　with Microsoft DirectX SDK (June 2010)  
Visual Studio2017
## 制作内容紹介
-DirectX１１のCompute Shader を利用しました.  
-Scott Le GrandによるBroad-Phase Collision Detection with CUDAのOptimized Grid Algorithmに基づいて、3DのGrid表を作ることにしました。  
-パーティクルのGrid表の作成、インデックスの順位付けについて、全てCompute Shaderで書いています。
## ゲームプレイ動画
[![Watch the video](https://www.youtube.com/watch?v=wNAorUrCHmM&feature=youtu.be)](https://youtu.be/wNAorUrCHmM)
