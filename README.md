# DirectX11 Present Hook by Tak
This project is a template for hooking D3D11's Present() function in an application using the Microsoft Detours library, and then drawing ImGui's demo window. This project compiles as a DLL that can be injected into any process.
If injected into a process that doesn't use DX11, the library will write an error message to its console window and then unload.
