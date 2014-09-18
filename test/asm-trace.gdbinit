set $i=0
break main
run
while ($i < 50000)
x/i $pc
si
set $i = $i + 1
end
quit
