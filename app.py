import streamlit as st
from google.cloud import firestore
from google.oauth2 import service_account
import pandas as pd
import datetime
import time

# 1. SETUP PAGE
st.set_page_config(page_title="Smart Air Monitor", page_icon="🇲🇾", layout="centered")

# 2. AUTHENTICATION
try:
    key_dict = service_account.Credentials.from_service_account_file('firestore-key.json')
    db = firestore.Client(credentials=key_dict, project="smart-air-gcp")
except Exception as e:
    st.error(f"Error loading key: {e}")
    st.stop()

# 3. HEADER
st.title("☁️ IoT Smart Air Monitor")
st.markdown("### CPC357 Assignment 2: Real-time Gas Detection")
st.markdown("**Location:** USM, Malaysia (UTC+8)")
st.divider()

# 4. METRICS LAYOUT
col1, col2, col3 = st.columns(3)

# 5. FETCH DATA LOGIC
# Fetch last 20 readings
docs = db.collection('sensor_readings')\
            .order_by('timestamp', direction=firestore.Query.DESCENDING)\
            .limit(20)\
            .stream()

data = []
# Define Malaysia Timezone (UTC +8)
MYT = datetime.timezone(datetime.timedelta(hours=8))

for doc in docs:
    d = doc.to_dict()
    
    # TIMESTAMP CONVERSION (Fix for Malaysia Time)
    if 'timestamp' in d and d['timestamp'] is not None:
        # Convert the Firestore timestamp to Malaysia Time
        local_time = d['timestamp'].astimezone(MYT)
        # Save as string for the chart
        d['timestamp_str'] = local_time.strftime('%H:%M:%S')
        # Keep original for sorting if needed
        d['real_time'] = local_time
    else:
        d['timestamp_str'] = "--:--:--"
    
    data.append(d)

# 6. DISPLAY DATA
if data:
    # LATEST READING (First item in list)
    latest = data[0]
    gas_val = latest.get('gas_value', 0)
    status = latest.get('status', 'N/A')
    last_time = latest.get('timestamp_str', 'Just now')

    # SHOW METRICS
    col1.metric("Gas Level (PPM)", gas_val)
    col2.metric("System Status", status)
    col3.metric("Last Updated", last_time)

    # DYNAMIC ALERT
    if status == "WARNING":
        st.error("🚨 **CRITICAL ALERT:** High Gas Levels Detected! Immediate action required.")
    else:
        st.success("✅ **SAFE:** Air quality is within normal parameters.")

    # CHART
    st.subheader("📉 Live Sensor Trend")
    df = pd.DataFrame(data)
    
    # Sort data so chart goes Old -> New (Left to Right)
    df = df.iloc[::-1] 
    
    if not df.empty:
        st.line_chart(df, x='timestamp_str', y='gas_value')
        
    # RAW DATA
    with st.expander("View Raw Data Logs"):
        st.dataframe(df[['timestamp_str', 'gas_value', 'status']])

else:
    st.info("Waiting for data from ESP32...")

# 7. AUTO-REFRESH LOGIC
# This creates a 10-second countdown bar and then reloads the page
st.divider()
st.caption("Auto-refreshing in 10 seconds...")
time.sleep(10)
st.rerun()

