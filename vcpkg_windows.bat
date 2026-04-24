mkdir build
cd build || goto :error
git clone https://github.com/Microsoft/vcpkg.git vcpkg.windows || goto :error
cd vcpkg.windows || goto :error
call bootstrap-vcpkg.bat || goto :error

vcpkg.exe install ^
	zlib:x64-windows-static ^
	boost-exception:x64-windows-static ^
	boost-program-options:x64-windows-static ^
	boost-stacktrace:x64-windows-static ^
	freetype:x64-windows-static ^
	glfw3:x64-windows-static ^
	glm:x64-windows-static ^
	vulkan:x64-windows-static ^
	freetype:x64-windows-static ^
	stb:x64-windows-static ^
	assimp:x64-windows-static ^
	nlohmann-json:x64-windows-static ^
	ffmpeg[core,avcodec,avformat,swscale,zlib,x264]:x64-windows-static ^
	|| goto :error

cd ..
cd ..

exit /b


:error
echo Failed with error #%errorlevel%.
exit /b %errorlevel%
