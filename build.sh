BUILD_PATH=build

cmake -B ${BUILD_PATH} \
	-S . \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=True

cp build/compile_commands.json .