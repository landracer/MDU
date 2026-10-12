#!/bin/bash
# Deep hardware debugging script for multidisplay-unified
# This script logs the actual raw data stream from ECU to identify data flow issues

echo "========================================"
echo "MultiDisplay Hardware Debug Script"
echo "========================================"

# Set up logging directory
LOG_DIR="./hardware_debug_logs"
mkdir -p "$LOG_DIR"

# Get current timestamp for log files
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_FILE="$LOG_DIR/hardware_debug_$TIMESTAMP.log"
RAW_LOG="$LOG_DIR/raw_data_$TIMESTAMP.log"

echo "Starting hardware debug session..."
echo "Log file: $LOG_FILE"
echo "Raw data log: $RAW_LOG"
echo "========================================"

# Function to get serial port info
get_port_info() {
    local port=$1
    echo "=== Port Information for $port ==="
    if [ -e "$port" ]; then
        echo "Port exists: $port"
        ls -la "$port"
        # Try to read basic info about the device
        if command -v stty &> /dev/null; then
            stty -F "$port" 2>/dev/null | head -10
        fi
    else
        echo "Port does not exist: $port"
    fi
    echo ""
}

# Function to run with detailed logging
run_with_logging() {
    local cmd="$1"
    local description="$2"
    
    echo "=== $description ===" >> "$LOG_FILE"
    echo "Command: $cmd" >> "$LOG_FILE"
    echo "Time: $(date)" >> "$LOG_FILE"
    echo "---" >> "$LOG_FILE"
    
    # Run the command with detailed output
    eval "$cmd" 2>&1 | tee -a "$LOG_FILE" | grep -E "(MD-ECU|ESP-NOW|frame|Frame|tag|payload|Data point)" | head -20
    
    echo "" >> "$LOG_FILE"
}

# Check if we have the right ports
echo "Checking available serial ports..."
ls -la /dev/ttyACM* 2>/dev/null || echo "No ACM ports found"
ls -la /dev/ttyUSB* 2>/dev/null || echo "No USB ports found"

# Get port information
get_port_info "/dev/ttyACM7"
get_port_info "/dev/ttyUSB2"

echo "========================================"
echo "Starting detailed hardware monitoring..."
echo "========================================"

# Run the application with verbose logging for 30 seconds
timeout 30s ./multidisplay-unified --md-ecu /dev/ttyACM7 --bridge /dev/ttyUSB2 2>&1 | \
while read line; do
    echo "$line" >> "$LOG_FILE"
    
    # Log raw data lines specifically
    if [[ "$line" =~ "Raw data received" ]] || [[ "$line" =~ "Frame detected" ]] || [[ "$line" =~ "Frame received" ]]; then
        echo "$line" >> "$RAW_LOG"
    fi
    
    # Show progress
    if [[ "$line" =~ "Connected" ]] || [[ "$line" =~ "Data point updated" ]] || [[ "$line" =~ "tag:" ]]; then
        echo "$line"
    fi
done

echo "========================================"
echo "Debug session completed"
echo "Log files created:"
echo "  - $LOG_FILE"
echo "  - $RAW_LOG"
echo "========================================"

# Show summary of what we found
echo ""
echo "=== SUMMARY ==="
echo "Check $LOG_FILE for detailed debug output"
echo "Check $RAW_LOG for raw frame data"
echo ""

# Show some key statistics from the logs
if [ -f "$RAW_LOG" ]; then
    echo "=== RAW DATA STATISTICS ==="
    echo "Total lines in raw log: $(wc -l < "$RAW_LOG")"
    echo "Frame detection lines: $(grep -c "Frame detected\|Frame received" "$RAW_LOG")"
    echo "Tag=0x5f frames: $(grep -c "tag.*0x5f\|tag.*5f" "$RAW_LOG")"
    echo "Tag=0x4f frames: $(grep -c "tag.*0x4f\|tag.*4f" "$RAW_LOG")"
    echo ""
fi