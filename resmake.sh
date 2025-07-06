list=("100" "544" "640" "800" "1088" "1280" "1440" "1680" "1920")
#cp link.txt build/CMakeFiles/hiddenchest.dir
cd build && clear && make && mv hi* ../bin/h${list[${1}]} && cd ..
