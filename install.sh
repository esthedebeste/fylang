set -e
cmake -B ./build . -G "Ninja Multi-Config"
cmake --build ./build --config Release
mkdir ~/.fy/bin -p
sudo cp ./build/fy ~/.fy/bin/
sudo rm ~/.fy/lib -rf
sudo cp ./lib ~/.fy/ -rf
extension="export PATH=\$PATH:~/.fy/bin"
grep -q "$extension" ~/.bashrc || echo "$extension" >> ~/.bashrc