#include "config.h"

#include <ros/ros.h>
#include <yaml-cpp/yaml.h>

Config::Config(const std::string& filename)
{
    auto root = YAML::LoadFile(filename);
    parseRoot(root);
}

Config::Config()
{
}

void Config::loadFromString(const std::string& input)
{
    auto root = YAML::Load(input);
    parseRoot(root);
}

tf2::Transform Config::parseTransform(const YAML::Node& node) const
{
    if (node["rot"].size() != 4)
        ROS_ERROR("Config: 'rot' is expected to have 4 elements");

    if (node["origin"].size() != 3)
        ROS_ERROR("Config: 'rot' is expected to have 4 elements");

    tf2::Quaternion rot;
    tf2::Vector3 origin;

    rot.setX(node["rot"][0].as<double>());
    rot.setY(node["rot"][1].as<double>());
    rot.setZ(node["rot"][2].as<double>());
    rot.setW(node["rot"][3].as<double>());

    origin.setX(node["origin"][0].as<double>());
    origin.setY(node["origin"][1].as<double>());
    origin.setZ(node["origin"][2].as<double>());

    return tf2::Transform(rot, origin);
}

void Config::parseRoot(const YAML::Node& node)
{
    if (!node)
        ROS_ERROR("Config: Document is empty");

    // quick sanity checks
    if (!node["entities"])
        ROS_ERROR("Config: Cannot find 'entities'");

    if (!node["markers"])
        ROS_ERROR("Config: Cannot find 'markers'");

    // load the entities
    for (const YAML::Node& entity : node["entities"])
    {
        Entity entityData;
        entityData.name = entity["name"].as<std::string>();

        // load the sensor data
        if (!entity["sensors"].IsNull())
        {
            for (const auto& sensor : entity["sensors"])
            {
                Sensor sensorData;
                sensorData.name   = sensor["name"].as<std::string>();
                sensorData.topic  = sensor["topic"].as<std::string>();
                sensorData.transf = parseTransform(sensor["transform"]);

                entityData.sensors.push_back(sensorData);
            }
        }

        m_entities.push_back(entityData);
    }

    // load the markers
    for (const YAML::Node& marker : node["markers"])
    {
        Marker markerData;
        markerData.id     = marker["id"].as<int>();
        markerData.ref    = marker["ref"].as<std::string>();
        markerData.transf = parseTransform(marker["transform"]);

        m_markers.push_back(markerData);
    }
}

std::vector<Marker> Config::markers() const
{
    return m_markers;
}

std::vector<Entity> Config::entities() const
{
    return m_entities;
}