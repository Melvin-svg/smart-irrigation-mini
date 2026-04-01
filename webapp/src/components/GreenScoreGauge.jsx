function GreenScoreGauge({ score }) {
  const isLoading = score === null || score === undefined;
  const safeScore = isLoading ? 0 : Math.min(100, Math.max(0, score));

  // SVG circle math
  const radius = 85;
  const circumference = 2 * Math.PI * radius;
  const dashArray = (safeScore / 100) * circumference;
  const dashOffset = circumference - dashArray;

  // Color based on score
  let strokeColor, statusText, statusClass;
  if (safeScore >= 65) {
    strokeColor = '#10b981';
    statusText = '🌿 Excellent Sustainability';
    statusClass = 'green-score__status--optimal';
  } else if (safeScore >= 35) {
    strokeColor = '#f59e0b';
    statusText = '⚠️ Moderate — Room for Improvement';
    statusClass = 'green-score__status--moderate';
  } else {
    strokeColor = '#ef4444';
    statusText = '🔴 Critical — Needs Attention';
    statusClass = 'green-score__status--critical';
  }

  const valueColorClass = safeScore >= 65 ? 'value-optimal' : safeScore >= 35 ? 'value-moderate' : 'value-critical';

  return (
    <div className="green-score-section">
      <div className="green-score__title">🏆 Green Score — Sustainability Index</div>

      <div className="green-score__gauge-wrapper">
        <svg className="green-score__gauge" viewBox="0 0 200 200">
          <circle
            className="green-score__gauge-bg"
            cx="100" cy="100" r={radius}
          />
          <circle
            className="green-score__gauge-fill"
            cx="100" cy="100" r={radius}
            stroke={strokeColor}
            strokeDasharray={`${dashArray} ${circumference}`}
            style={{ transition: 'stroke-dasharray 1s ease, stroke 0.5s ease' }}
          />
        </svg>
        <div className="green-score__gauge-value">
          {isLoading ? (
            <span className="loading-skeleton" style={{ width: 60, height: 48, display: 'inline-block' }}></span>
          ) : (
            <>
              <div className={`green-score__number ${valueColorClass}`}>{Math.round(safeScore)}</div>
              <div className="green-score__label">out of 100</div>
            </>
          )}
        </div>
      </div>

      {!isLoading && (
        <div className={`green-score__status ${statusClass}`}>{statusText}</div>
      )}
    </div>
  );
}

export default GreenScoreGauge;
