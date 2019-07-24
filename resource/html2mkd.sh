#!/usr/bin/env bash
for i in `ls *.html | tr " " "\?"`;do
    pandoc -o "${i%.html}.md" "${i}"
done