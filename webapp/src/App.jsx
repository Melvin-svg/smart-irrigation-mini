import { useState, useEffect } from 'react';
import { database, ref, onValue } from './firebase';
import SensorCard from './components/SensorCard';
import GreenScoreGauge from './components/GreenScoreGauge';
import PumpControl from './components/PumpControl';

function App() {
  const [sensorData, setSensorData] = useState({
    moisture: null,
    humidity: null,
    temperature: null,
    timestamp: null,
  });
  const [control, setControl] = useState({
    pump: null,
    pumpStatus: null,
  });
  const [greenScore, setGreenScore] = useState(null);
  const [connected, setConnected] = useState(false);
  const [lastUpdated, setLastUpdated] = useState(null);

  // Listen to Firebase Realtime Database
  useEffect(() => {
    // Sensor data listener
    const sensorRef = ref(database, '/sensorData');
    const unsubSensor = onValue(sensorRef, (snapshot) => {
      const data = snapshot.val();
      if (data) {
        setSensorData(data);
        setLastUpdated(new Date());
        setConnected(true);
      }
    }, (error) => {
      console.error('Sensor data error:', error);
      setConnected(false);
    });

    // Control listener
    const controlRef = ref(database, '/control');
    const unsubControl = onValue(controlRef, (snapshot) => {
      const data = snapshot.val();
      if (data) {
        setControl(data);
      }
    });

    // Green score listener
    const metricsRef = ref(database, '/metrics');
    const unsubMetrics = onValue(metricsRef, (snapshot) => {
      const data = snapshot.val();
      if (data) {
        setGreenScore(data.greenScore);
      }
    });

    return () => {
      unsubSensor();
      unsubControl();
      unsubMetrics();
    };
  }, []);

  // Determine color classes based on values
  const getMoistureColor = (val) => {
    if (val === null) return '';
    if (val >= 50) return 'value-optimal';
    if (val >= 25) return 'value-moderate';
    return 'value-critical';
  };

  const getHumidityColor = (val) => {
    if (val === null) return '';
    if (val >= 40) return 'value-optimal';
    if (val >= 20) return 'value-moderate';
    return 'value-critical';
  };

  const getTemperatureColor = (val) => {
    if (val === null) return '';
    if (val >= 15 && val <= 35) return 'value-optimal';
    if (val >= 10 && val <= 40) return 'value-moderate';
    return 'value-critical';
  };

  const formatTime = (date) => {
    if (!date) return 'Waiting for data...';
    return date.toLocaleTimeString('en-US', {
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit',
    });
  };

  return (
    <div className="app-container">
      {/* Header */}
      <header className="header" id="dashboard-header">
        <span className="header__icon">🌱</span>
        <h1 className="header__title">Smart Irrigation System</h1>
        <p className="header__subtitle">Real-time monitoring &amp; control dashboard</p>
        <div className={`connection-badge ${connected ? '' : 'offline'}`}>
          <span className="connection-dot"></span>
          {connected ? 'Connected to ESP32' : 'Waiting for connection...'}
        </div>
      </header>

      {/* Sensor Cards Grid */}
      <div className="dashboard-grid" id="sensor-grid">
        <SensorCard
          label="Soil Moisture"
          icon="🌿"
          value={sensorData.moisture}
          unit="%"
          progress={sensorData.moisture}
          colorClass={getMoistureColor(sensorData.moisture)}
          progressBarClass="sensor-card__progress-bar--blue"
          cardClass="sensor-card--moisture"
        />
        <SensorCard
          label="Humidity"
          icon="💧"
          value={sensorData.humidity}
          unit="%"
          progress={sensorData.humidity}
          colorClass={getHumidityColor(sensorData.humidity)}
          progressBarClass="sensor-card__progress-bar--purple"
          cardClass="sensor-card--humidity"
        />
        <SensorCard
          label="Temperature"
          icon="🌡️"
          value={sensorData.temperature}
          unit="°C"
          progress={sensorData.temperature ? (sensorData.temperature / 50) * 100 : 0}
          colorClass={getTemperatureColor(sensorData.temperature)}
          progressBarClass="sensor-card__progress-bar--amber"
          cardClass="sensor-card--temperature"
        />
        <SensorCard
          label="Pump"
          icon="⚡"
          value={control.pumpStatus ? 'ON' : 'OFF'}
          unit=""
          progress={control.pumpStatus ? 100 : 0}
          colorClass={control.pumpStatus ? 'value-optimal' : 'value-critical'}
          progressBarClass="sensor-card__progress-bar--green"
          cardClass="sensor-card--pump"
        />
      </div>

      {/* Green Score Gauge */}
      <GreenScoreGauge score={greenScore} />

      {/* Pump Control */}
      <PumpControl pumpOn={control.pump} pumpStatus={control.pumpStatus} />

      {/* Last Updated */}
      <div className="last-updated" id="last-updated">
        Last updated: {formatTime(lastUpdated)}
      </div>
    </div>
  );
}

export default App;
