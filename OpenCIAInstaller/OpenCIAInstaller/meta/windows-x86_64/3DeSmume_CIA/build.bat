emake
ebannertool makebanner -i "cia/banner.png" -a "cia/audio.wav" -o "cia/banner.bnr"
ebannertool makesmdh -i "cia/icon.png" -s "DS on 3DS emulator" -l "3DeSmume" -p "Shutterbug2000" -o "cia/icon.icn"
makerom -f cia -o e.cia -rsf cia/template.rsf -target t -elf e.elf -icon cia/icon.icn -banner cia/banner.bnr
pause