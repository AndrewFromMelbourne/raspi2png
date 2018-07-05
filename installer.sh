sudo apt-get -y install libpng12-dev
cd ~
git clone https://github.com/AndrewFromMelbourne/raspi2png.git
cd raspi2png
sudo make install
cd ..
rm -fr raspi2png
raspi2png -H
