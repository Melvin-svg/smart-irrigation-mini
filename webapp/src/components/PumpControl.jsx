import { database, ref, set } from '../firebase';

function PumpControl({ pumpOn, pumpStatus, manualMode }) {
  const isLoading = pumpOn === null || pumpOn === undefined;

  const setPump = async (value) => {
    if (!manualMode) {
      console.warn('Pump control disabled in AUTO mode');
      return;
    }
    try {
      console.log(`Setting pump to: ${value}`);
      const pumpRef = ref(database, '/control/motor');
      await set(pumpRef, value);
    } catch (error) {
      console.error('Failed to set pump:', error);
    }
  };

  const actuallyRunning = pumpOn === true;

  return (
    <div className="pump-control-section">
      <div className="pump-control__title">💧 Irrigation Control Panel</div>

      <div className="pump-control__toggle-wrapper">
        <span 
          className={`pump-control__label ${!pumpOn ? 'pump-control__label--active' : ''}`}
          onClick={() => setPump(false)}
          style={{ cursor: manualMode ? 'pointer' : 'default' }}
        >
          OFF
        </span>
        <label className="toggle-switch" id="pump-toggle">
          <input
            type="checkbox"
            checked={pumpOn || false}
            onChange={() => setPump(!pumpOn)}
            disabled={isLoading || !manualMode}
          />
          <span className="toggle-slider"></span>
        </label>
        <span 
          className={`pump-control__label ${pumpOn ? 'pump-control__label--active' : ''}`}
          onClick={() => setPump(true)}
          style={{ cursor: manualMode ? 'pointer' : 'default' }}
        >
          ON
        </span>
      </div>

      <div className={`pump-control__status ${actuallyRunning ? 'pump-control__status--on' : 'pump-control__status--off'}`}>
        <span className="pump-status-icon">{actuallyRunning ? '💧' : '🛑'}</span>
        {isLoading ? 'Connecting...' : actuallyRunning ? 'Pump is Running' : 'Pump is Stopped'}
      </div>

      <div className="pump-control__mode-info">
        {manualMode ? (
          <span className="mode-tag mode-tag--manual">✋ Manual Override Active</span>
        ) : (
          <span className="mode-tag mode-tag--auto">🤖 Automatic Logic Active — Toggle disabled</span>
        )}
      </div>
    </div>
  );
}

export default PumpControl;
