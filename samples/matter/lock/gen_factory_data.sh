
#!/bin/bash

set -e
cd $(dirname "$0")

for suffix in "_1" "_2"; do
  rm -rf build
  west build -b nrf5340dk_nrf5340_cpuapp -t factory_data_hex -- -DOVERLAY_CONFIG="overlay-factory_data_build${suffix}.conf"

  mkdir -p factory_data${suffix}
  cp build/zephyr/factory_data* factory_data${suffix}
done
