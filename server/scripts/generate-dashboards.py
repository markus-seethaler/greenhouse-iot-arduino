#!/usr/bin/env python3
"""
Grafana Dashboard Generator for Greenhouse IoT

Reads sensor registry from config/sensors.yml and generates
one Grafana dashboard JSON per location.

Usage:
    cd server
    python scripts/generate-dashboards.py
"""

import json
import os
import sys
from collections import defaultdict
from pathlib import Path

import yaml

# Paths relative to server directory
SCRIPT_DIR = Path(__file__).parent
SERVER_DIR = SCRIPT_DIR.parent
CONFIG_FILE = SERVER_DIR / "config" / "sensors.yml"
DASHBOARDS_DIR = SERVER_DIR / "grafana" / "provisioning" / "dashboards"

# Sensor type configurations
SENSOR_CONFIGS = {
    "soil_moisture": {
        "unit": "percent",
        "thresholds": [
            {"color": "red", "value": None},
            {"color": "yellow", "value": 30},
            {"color": "green", "value": 50},
        ],
        "graph_color": "green",
    },
    "temperature": {
        "unit": "celsius",
        "thresholds": [
            {"color": "blue", "value": None},
            {"color": "green", "value": 18},
            {"color": "yellow", "value": 28},
            {"color": "red", "value": 35},
        ],
        "graph_color": "orange",
    },
    "humidity": {
        "unit": "percent",
        "thresholds": [
            {"color": "yellow", "value": None},
            {"color": "green", "value": 40},
            {"color": "blue", "value": 80},
        ],
        "graph_color": "blue",
    },
    "air_quality": {
        "unit": "concentrationpm25",
        "thresholds": [
            {"color": "green", "value": None},
            {"color": "yellow", "value": 12},
            {"color": "orange", "value": 35},
            {"color": "red", "value": 55},
        ],
        "graph_color": "purple",
    },
}


def load_config():
    """Load and parse sensors.yml"""
    if not CONFIG_FILE.exists():
        print(f"Error: Config file not found: {CONFIG_FILE}")
        sys.exit(1)

    with open(CONFIG_FILE) as f:
        return yaml.safe_load(f)


def group_by_location(devices):
    """Group devices by their location"""
    locations = defaultdict(list)
    for device_id, device_config in devices.items():
        location = device_config.get("location", "unknown")
        locations[location].append({
            "id": device_id,
            **device_config,
        })
    return locations


def create_stat_panel(panel_id, sensor_field, sensor_config, device_id, location, x_pos, y_pos):
    """Create a stat panel for a single sensor"""
    sensor_type = sensor_config.get("type", "soil_moisture")
    type_config = SENSOR_CONFIGS.get(sensor_type, SENSOR_CONFIGS["soil_moisture"])

    return {
        "id": panel_id,
        "title": sensor_config.get("name", sensor_field),
        "type": "stat",
        "gridPos": {"x": x_pos, "y": y_pos, "w": 6, "h": 4},
        "datasource": {"type": "influxdb", "uid": "influxdb"},
        "fieldConfig": {
            "defaults": {
                "unit": type_config["unit"],
                "thresholds": {
                    "mode": "absolute",
                    "steps": type_config["thresholds"],
                },
            }
        },
        "options": {
            "colorMode": "background",
            "graphMode": "none",
            "textMode": "auto",
        },
        "targets": [
            {
                "refId": "A",
                "query": f'''from(bucket: "greenhouse")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "greenhouse")
  |> filter(fn: (r) => r._field == "{sensor_field}")
  |> filter(fn: (r) => r.device_id == "{device_id}")
  |> filter(fn: (r) => r.location == "{location}")
  |> last()''',
            }
        ],
    }


def create_history_panel(panel_id, device_id, location, sensors, y_pos):
    """Create a time-series history panel for a device"""
    sensor_fields = list(sensors.keys())
    field_filter = " or ".join([f'r._field == "{f}"' for f in sensor_fields])

    # Build field overrides for colors and units
    overrides = []
    for field, config in sensors.items():
        sensor_type = config.get("type", "soil_moisture")
        type_config = SENSOR_CONFIGS.get(sensor_type, SENSOR_CONFIGS["soil_moisture"])
        overrides.append({
            "matcher": {"id": "byName", "options": field},
            "properties": [
                {"id": "unit", "value": type_config["unit"]},
                {"id": "color", "value": {"fixedColor": type_config["graph_color"], "mode": "fixed"}},
                {"id": "displayName", "value": config.get("name", field)},
            ],
        })

    return {
        "id": panel_id,
        "title": "Sensor History",
        "type": "timeseries",
        "gridPos": {"x": 0, "y": y_pos, "w": 24, "h": 8},
        "datasource": {"type": "influxdb", "uid": "influxdb"},
        "fieldConfig": {
            "defaults": {
                "custom": {
                    "lineWidth": 2,
                    "fillOpacity": 10,
                    "pointSize": 5,
                    "showPoints": "auto",
                }
            },
            "overrides": overrides,
        },
        "options": {
            "legend": {
                "displayMode": "list",
                "placement": "bottom",
                "showLegend": True,
            },
            "tooltip": {"mode": "multi"},
        },
        "targets": [
            {
                "refId": "A",
                "query": f'''from(bucket: "greenhouse")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r._measurement == "greenhouse")
  |> filter(fn: (r) => r.device_id == "{device_id}")
  |> filter(fn: (r) => r.location == "{location}")
  |> filter(fn: (r) => {field_filter})''',
            }
        ],
    }


def create_device_row(panel_id, device, y_pos):
    """Create a collapsible row for a device"""
    return {
        "id": panel_id,
        "title": f"{device['id']} - {device.get('description', '')}",
        "type": "row",
        "gridPos": {"x": 0, "y": y_pos, "w": 24, "h": 1},
        "collapsed": False,
    }


def generate_dashboard(location, devices):
    """Generate a complete dashboard for a location"""
    panels = []
    panel_id = 1
    y_pos = 0

    for device in devices:
        device_id = device["id"]
        sensors = device.get("sensors", {})

        # Add device row header
        panels.append(create_device_row(panel_id, device, y_pos))
        panel_id += 1
        y_pos += 1

        # Add stat panels for each sensor (4 per row)
        x_pos = 0
        for sensor_field, sensor_config in sensors.items():
            panels.append(create_stat_panel(
                panel_id, sensor_field, sensor_config,
                device_id, location, x_pos, y_pos
            ))
            panel_id += 1
            x_pos += 6
            if x_pos >= 24:
                x_pos = 0
                y_pos += 4

        # Move to next row if we didn't fill the last one
        if x_pos > 0:
            y_pos += 4

        # Add history panel
        panels.append(create_history_panel(
            panel_id, device_id, location, sensors, y_pos
        ))
        panel_id += 1
        y_pos += 8

    return {
        "title": location.replace("-", " ").replace("_", " ").title(),
        "uid": f"greenhouse-{location}",
        "editable": True,
        "panels": panels,
        "schemaVersion": 39,
        "version": 1,
        "refresh": "10s",
    }


def main():
    config = load_config()
    devices = config.get("devices", {})

    if not devices:
        print("No devices found in config")
        sys.exit(1)

    # Group devices by location
    locations = group_by_location(devices)

    # Ensure dashboards directory exists
    DASHBOARDS_DIR.mkdir(parents=True, exist_ok=True)

    # Generate one dashboard per location
    generated = []
    for location, location_devices in locations.items():
        dashboard = generate_dashboard(location, location_devices)
        output_file = DASHBOARDS_DIR / f"greenhouse-{location}.json"

        with open(output_file, "w") as f:
            json.dump(dashboard, f, indent=2)

        generated.append(output_file.name)
        print(f"Generated: {output_file}")

    print(f"\nGenerated {len(generated)} dashboard(s): {', '.join(generated)}")


if __name__ == "__main__":
    main()
