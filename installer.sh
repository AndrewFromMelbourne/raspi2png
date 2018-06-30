sudo apt-get -y install libpng12-dev
cd ~
git clone https://github.com/AndrewFromMelbourne/raspi2png.git
cd raspi2png
sudo make
sudo chmod 755 raspi2png
sudo mv raspi2png /usr/bin
cd ..
rm -fr raspi2png
raspi2png -H
