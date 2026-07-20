list=("100" "544" "640" "800" "1088" "1280" "1440" "1680" "1920")
#cp link.txt build/CMakeFiles/hiddenchest.dir
for i in {1..8}; do
  printf "The resolution is: %4d\n" ${list[${i}]}
  sed -i "1s/[0-9]/$i/" src/resolution.h
  cd build && clear && make && mv hi* ../bin/h${list[${i}]} && cd ..
done
exit 0
