FROM lscr.io/linuxserver/webtop:ubuntu-xfce
USER root
RUN sudo apt-get update && sudo apt-get install -y libglew-dev libxext-dev freeglut3-dev uuid-dev python3-pip 
RUN sudo pip install --break-system-packages PyYAML Pillow shortuuid

# FROM ubuntu:24.04
# RUN apt-get update && apt-get install -y libglew-dev libxext-dev freeglut3-dev uuid-dev python3-pip cmake git
# RUN pip install --break-system-packages PyYAML Pillow shortuuid

# COPY . /config/Desktop/farmville
# RUN rm -rf /config/Desktop/farmville/.git
# RUN rm -rf /config/Desktop/farmville/build
# RUN chmod -R 777 /config/Desktop/farmville
# RUN chown -R abc:abc /config/Desktop/farmville
# RUN cd /config/Desktop/farmville && sudo ./compile.sh

# RUN echo '[Desktop Entry]\nType=Application\nExec=bash -c "cd /config/Desktop/farmville/build/cmake/cmake/install/ && ./Farmville.exe"\nName=FarmvilleApp' > /config/Desktop/FarmvilleApp.desktop
# RUN chmod +x /config/Desktop/FarmvilleApp.desktop
# RUN chown abc:abc /config/Desktop/FarmvilleApp.desktop