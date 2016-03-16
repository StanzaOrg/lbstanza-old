if [ $# -lt 1 ]; then
    echo "Not enough arguments"
    exit 2
fi
hg archive $1
mv -f $1/notes $1/scripts $1/tests ~/.Trash/
mv lstanza $1/lstanza
cd $1
zip -r stanza.zip compiler core docs runtime stanza
mv stanza stanzabak
mv lstanza stanza
zip -r lstanza.zip compiler core docs runtime stanza
mv -f compiler core docs runtime stanza stanzabak ~/.Trash/
