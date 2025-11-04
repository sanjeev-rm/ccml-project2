#!/usr/bin/env bash
SKIP_CUGL=false
for arg in "$@"; do
  if [[ "$arg" == "skip-cugl" ]]; then
    SKIP_CUGL=true
    break
  fi
done

if ! $SKIP_CUGL; then
  echo "Running cugl build..."
  python3 cugl .
else
  echo "Skipping cugl build."
fi

cd build/cmake/cmake &&
#cmake -DCUGL_AUDIO=OFF .. &&
cmake .. &&
cmake --build . -j 4