#!/bin/bash

# Function to stop VNC and websockify
stop_vnc() {
    echo "Stopping VNC and websockify processes..."
    # 使用sudo终止websockify进程
    sudo pkill -f websockify || echo "No websockify processes found"
    # 检查端口占用情况并强制终止
    WEBSOCKIFY_PID=$(sudo lsof -t -i:6080 2>/dev/null)
    if [ ! -z "$WEBSOCKIFY_PID" ]; then
        echo "Killing process on port 6080: $WEBSOCKIFY_PID"
        sudo kill -9 $WEBSOCKIFY_PID 2>/dev/null || true
    fi
    
    vncserver -kill :1 || echo "No VNC server running on display :1"
    rm -rf /tmp/.X1-lock /tmp/.X11-unix/X1 2>/dev/null || true
    
    # 确保端口6080空闲
    sleep 2
}

# Function to start VNC and websockify
start_vnc() {
    # Get current user
    CURRENT_USER=$(whoami)
    USER_HOME=$(eval echo ~$CURRENT_USER)

    # Setup VNC password if not exists
    if [ ! -f "$USER_HOME/.vnc/passwd" ]; then
        mkdir -p $USER_HOME/.vnc
        echo "password" | vncpasswd -f > $USER_HOME/.vnc/passwd
        chmod 600 $USER_HOME/.vnc/passwd
    fi

    # Create VNC configuration if not exists
    if [ ! -f "$USER_HOME/.vnc/xstartup" ]; then
        cat > $USER_HOME/.vnc/xstartup << EOF
#!/bin/bash
xrdb $USER_HOME/.Xresources

# Set Firefox as default browser
xdg-settings set default-web-browser firefox.desktop

# Start desktop environment
startxfce4 &
EOF
        chmod +x $USER_HOME/.vnc/xstartup
    fi

    # 检查并关闭可能已经在运行的VNC服务器
    vncserver -kill :1 >/dev/null 2>&1 || true
    
    # 等待资源释放
    sleep 2
    
    # Start VNC server
    vncserver :1 -geometry 1280x800 -depth 24
    
    # 确保端口6080没有被占用
    if sudo lsof -i:6080 >/dev/null 2>&1; then
        echo "Port 6080 is already in use. Stopping the process..."
        sudo kill -9 $(sudo lsof -t -i:6080) 2>/dev/null
        sleep 2
    fi

    # Setup noVNC with sudo
    echo "Starting websockify..."
    sudo websockify -D --web=/usr/share/novnc/ 6080 localhost:5901

    echo "Desktop environment has been set up!"
    echo "You can access it through noVNC at: http://localhost:6080"
}

# Check command line arguments
case "$1" in
    restart)
        stop_vnc
        start_vnc
        ;;
    stop)
        stop_vnc
        ;;
    start)
        start_vnc
        ;;
    *)
        # First time setup
        # Update package lists
        sudo apt-get update

        # Install Ubuntu desktop, VNC server, and other necessary packages
        sudo DEBIAN_FRONTEND=noninteractive apt-get install -y \
            ubuntu-desktop \
            tightvncserver \
            xfce4 \
            xfce4-goodies \
            novnc \
            net-tools \
            websockify \
            lsof

        start_vnc
        ;;
esac