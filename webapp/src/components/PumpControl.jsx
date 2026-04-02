import { database, ref, set } from '../firebase';

function PumpControl({ pumpOn, pumpStatus, manualMode }) {
  const isLoading = pumpOn === null || pumpOn === undefined;

  const handleToggle = async () => {
    if (!manualMode) {
      console.warn('Pump toggle disabled in AUTO mode');
      return;
    }
    try {
      const pumpRef = ref(database, '/control/motor');
      await set(pumpRef, !pumpOn);
    } catch (error) {
      console.error('Failed to toggle pump:', error);
    }
  };

  // pumpStatus = actual pump state from ESP32
  // pumpOn = commanded state from web app
  const actuallyRunning = pumpStatus === true;

  return (
    <div className="pump-control-section">
      <div className="pump-control__title">💧 Irrigation Control Panel</div>

      <div className="pump-control__toggle-wrapper">
        <span className={`pump-control__label ${!pumpOn ? 'pump-control__label--active' : ''}`}>OFF</span>
        <label className="toggle-switch" id="pump-toggle">
          <input
            type="checkbox"
            checked={pumpOn || false}
            onChange={handleToggle}
            disabled={isLoading || !manualMode}
          />
          <span className="toggle-slider"></span>
        </label>
        <span className={`pump-control__label ${pumpOn ? 'pump-control__label--active' : ''}`}>ON</span>
      </div>

      <div className={`pump-control__status ${actuallyRunning ? 'pump-control__status--on' : 'pump-control__status--off'}`}>
        <span className="pump-status-icon">{actuallyRunning ? '💧' : '🛑'}</span>
        {isLoading ? 'Connecting...' : actuallyRunning ? 'Pump is Running' : 'Pump is Stopped'}
      </div>

      <div className="pump-control__mode-info">
        {manualMode ? (
          <span className="mode-tag mode-tag--manual">✋ Manual Override Active</span>
        ) : (
          <span className="mode-tag mode-tag--auto">🤖 Automatic Logic Active</span>
        )}
      </div>
    </div>
  );
}

export default PumpControl;
