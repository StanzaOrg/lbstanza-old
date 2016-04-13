if [ $# -lt 1 ]; then
    echo "Not enough arguments"
    exit 2
fi
hg archive $1
cp lstanza $1/lstanza
cd $1
zip -r stanza.zip compiler core docs runtime License.txt stanza
mv lstanza stanza
zip -r lstanza.zip compiler core docs runtime License.txt stanza
mv stanza.zip lstanza.zip ~/Desktop/
rm *
