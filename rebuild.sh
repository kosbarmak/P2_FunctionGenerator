#!/usr/bin/env bash

# Configure the Debug build before compiling its firmware target
cmake --fresh --preset Debug
configure_status=$?

if [ "$configure_status" -eq 0 ]; then
  cmake --build --preset Debug
  build_status=$?
else
  build_status=$configure_status
fi

# Keep a separately opened terminal visible so its output can be reviewed
printf '\nBuild exited with status %s.\n' "$build_status"
read -r -p "Press Enter to close..."

exit "$build_status"
