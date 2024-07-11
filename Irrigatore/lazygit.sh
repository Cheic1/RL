#bash
# Usare questi script per utilizzare pio
# mkdir -p /usr/local/bin
# sudo ln -s ~/.platformio/penv/bin/platformio /usr/local/bin/platformio
# sduo ln -s ~/.platformio/penv/bin/pio /usr/local/bin/pio
# sudo ln -s ~/.platformio/penv/bin/piodebuggdb /usr/local/bin/piodebuggdb
# cd ..
pio run --environment d1
git add .
git commit -a -m "$1"
git push