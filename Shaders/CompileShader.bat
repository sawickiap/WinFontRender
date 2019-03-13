set FXC_PATH="c:\Program Files (x86)\Windows Kits\10\bin\10.0.17763.0\x64\fxc.exe"
%FXC_PATH% /O3 Shader.hlsl /T vs_5_0 /E MainVS /Fo CompiledShader_VS.dxbc
%FXC_PATH% /O3 Shader.hlsl /T ps_5_0 /E MainPS /Fo CompiledShader_PS.dxbc
