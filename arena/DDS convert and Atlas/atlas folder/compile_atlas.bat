sed "s/	0//g" atlas.def > atlas.part1
sed "s/	//g" atlas.part1 > atlas.part2
sed "/atlas/d" atlas.part2 > atlas.part3
sed "s/{//g" atlas.part3 > atlas.part4
sed "s/}//g" atlas.part4 > atlas.part5
sed "/^$/d" atlas.part5 > atlas.part6
sed "s/.png//g" atlas.part6 > atlas.part7
echo. >> atlas.part7
sed "s/ //g" atlas.part7 > new_atlas.manifest

del atlas.manifest

move new_atlas.manifest atlas.manifest
move /Y atlas.png atlas.dat

copy atlas.manifest ..\..\assets\
copy atlas.dat ..\..\assets\

del /f atlas.part*

timeout 1
