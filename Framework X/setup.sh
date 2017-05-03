cd 3rd/json-schema-validator
mkdir build
cd build
cmake -DNLOHMANN_JSON_DIR=../../json .. && make
cd ../../../..
