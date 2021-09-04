#!/bin/bash
rm -rf release
mkdir -p release

cp -rf Sampoler *.{hpp,cpp,txt,json} LICENSE release/

mv release score-addon-sampoler
7z a score-addon-sampoler.zip score-addon-sampoler
