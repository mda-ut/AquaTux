# Kill nicely
pkill -2 aquatux
sleep 1
#Force kill
pkill -9 aquatux
pkill -9 nios2-terminal
pkill -9 nios2-terminal-wrapped
echo "You should now be able to rerun the sub test without problems"
