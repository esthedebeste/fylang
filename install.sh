set +e
cmake -B ./build . -DCMAKE_BUILD_TYPE=Release
cmake --build ./build
mkdir ~/.fy/bin -p
sudo cp ./build/fy ~/.fy/bin/
sudo cp ./lib/**.fy ~/.fy/lib/ -rf
extension="export PATH=\$PATH:~/.fy/bin"
grep -q "$extension" ~/.bashrc || echo "$extension" >> ~/.bashrc