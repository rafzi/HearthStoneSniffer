for %%i in (*.proto) do protoc --cpp_out=.. %%i
pause
