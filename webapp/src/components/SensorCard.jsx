import { useState, useEffect } from 'react';

function SensorCard({ label, icon, value, unit, progress, colorClass, progressBarClass, cardClass }) {
  const isLoading = value === null || value === undefined;

  return (
    <div className={`sensor-card ${cardClass || ''}`}>
      <div className="sensor-card__header">
        <div className="sensor-card__label">
          <span className="sensor-card__icon">{icon}</span>
          {label}
        </div>
        <div className="sensor-card__status-dot"></div>
      </div>
      <div className={`sensor-card__value ${colorClass || ''}`}>
        {isLoading ? (
          <span className="loading-skeleton loading-value"></span>
        ) : (
          <>
            {typeof value === 'number' ? value.toFixed(1) : value}
            <span className="sensor-card__unit">{unit}</span>
          </>
        )}
      </div>
      {progress !== undefined && progress !== null && (
        <div className="sensor-card__progress">
          <div
            className={`sensor-card__progress-bar ${progressBarClass || ''}`}
            style={{ width: `${Math.min(100, Math.max(0, progress))}%` }}
          ></div>
        </div>
      )}
    </div>
  );
}

export default SensorCard;
