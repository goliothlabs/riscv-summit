Set up Zephyr

```
cd ~
mkdir riscv-summit
python3 -m venv riscv-summit/.venv
source riscv-summit/.venv/bin/activate
pip install wheel
pip install west
```

Get the sample

```
cd ~
west init -m https://github.com/beriberikix/riscv-summit riscv-summit
cd riscv-summit
west update
west zephyr-export
pip install -r ~/riscv-summit/deps/zephyr/scripts/requirements.txt
west blobs fetch hal_espressif
```