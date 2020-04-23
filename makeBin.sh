#!/bin/bash
rm -r Bin
mkdir Bin
cp -r Agent/Lib/lib Bin
cp Agent/src/sample_player Bin
cp Agent/src/sample_coach Bin
cp Agent/src/player.conf Bin
cp Agent/src/coach.conf Bin
cp -r Agent/src/formations-dt Bin
cp -r Agent/src/heliusFor Bin
cp Files/* Bin
tar czf Binary.tar.gz Bin
mv Binary.tar.gz Bin
