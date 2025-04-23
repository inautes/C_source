#!/bin/sh

test $# -eq 2 || exit

echo "string ${1} -> ${2}"

list=`/bin/ls -1 *.c* *.h* make* mk* 2>/dev/null`
nmod=0

for i in $list
do
n=`cat $i | fgrep ${1} | wc -l`
n=`echo ${n}`
if test ${n} -ge 1 ; then
cat ${i} | sed "s/${1}/${2}/g" > ${i}.tmp
echo ${n} substitutions in ${i}
nmod=`expr $n + 1`
fi
done

if test $nmod -eq 0 ; then
echo "nothing to change"
exit
fi

echo "do you want continue? (y/n)"
read ans

answer=${ans:-y}

if test ${answer} = "y" ; then
for i in *.tmp
do
name=`basename ${i} ''.tmp''`
cp ${i} ${name}
/bin/rm -f ${i}
done
else
/bin/rm -f *.tmp
fi
