/**
 * @file Serializer.hh
 * @brief Class responsible for text entries serialization.
 * @author Reinaldo Silva
 * @version 1.0
 * @date 2013
 * @copyright Copyright 2013 Produban. All rights reserved.
 * @copyright Licensed under the Apache License, Version 2.0
 * @copyright http://www.apache.org/licenses/LICENSE-2.0
 */

/*
 * Copyright 2013 Produban
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#ifndef _LOG2KAFKA_SERIALIZER_HH_
#define _LOG2KAFKA_SERIALIZER_HH_

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>

#include <avro/AvroSerialize.hh>
#include <avro/Compiler.hh>
#include <avro/DataFile.hh>
#include <avro/Decoder.hh>
#include <avro/Encoder.hh>
#include <avro/Generic.hh>
#include <avro/Node.hh>
#include <avro/Specific.hh>
#include <avro/Types.hh>
#include <avro/Writer.hh>

#include <boost/array.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/xpressive/xpressive.hpp>

#include "Mapper.hh"

typedef boost::array<uint8_t, 4> Magic;
typedef boost::array<uint8_t, 16> DataBlockSync;
typedef std::map<std::string, std::vector<uint8_t>> Metadata;

/**
 * Class responsible for serializing text entries.
 */
class Serializer {
public:

    Serializer();
    virtual ~Serializer();

    /**
     * Class constructor with direct schema definition initialization.
     *
     * After setting the schema configuration file path, the constructor
     * invoke the #configure() method.
     *
     * @param configFilePath the file path to the schema configuration
     *                       and mapping
     */
    explicit Serializer(std::string configFilePath);

    /*-- getters/setters --*/

    /**
     * Set the configuration file path.
     */
    void configFilePath(std::string configFilePath);

    /**
     * Return the configuration file path.
     */
    const std::string& configFilePath() const;

    /*-- methods --*/

    /**
     * Read de configuration file and load the schema mapper to use for
     * serialization.
     */
    void configure();

    /**
     * Serialize a input text using the schema and mapper defined for the
     * instance.
     *
     * @param[in] entry The input text to serialize
     * @param[out] data The output data buffer
     */
    void serialize(const std::string& entry, std::auto_ptr<avro::OutputStream>& data);

private:

    /*-- static fields --*/

#ifdef _LOG2KAFKA_USE_LOG4CXX_
    /**
     * Class logger.
     */
    static log4cxx::LoggerPtr logger;
#endif

    /**
     * Text that mark the beginning of the AVRO schema definition in the
     * configuration file.
     */
    static const std::string schemaMarker;

    /*-- fields --*/

    /**
     * Schema and pattern mapper configuration file.
     */
    std::string configFilePath_;

    /**
     * AVRO Schema mapper.
     */
    Mapper mapper_;

    /**
     * Avro data block sync marker.
     */
    DataBlockSync sync_;

    /**
     * Avro metadata key-value pairs.
     */
    Metadata metadata_;

    /*-- methods --*/

    /**
     * Load a schema mapper according to the definition read from the input
     * stream.
     *
     * @param is the input stream to read.
     */
    void loadMapper(std::istream &is);

    /**
     * Write the Avro serialized message header.
     */
    void writeHeader(avro::EncoderPtr& e);

    /**
     * Write the Avro serialized message data block.
     */
    void writeDataBlock(avro::EncoderPtr& e, const avro::GenericDatum& datum, int64_t byteCount);

    /**
     * Set an Avro metadata key-value pair.
     *
     * @param key the metadata key name
     * @param value the metadata key value
     */
    void setMetadata(const std::string& key, const std::string& value);

    /**
     *  Calculate and return a Avro data block sync marker.
     */
    DataBlockSync makeSync();

    /**
     * Build a temporal file name for persistent the serialized message.
     * Used in trace mode for debugging purposes.
     */
    const char* buildTempFileName();

    /**
     * Display schema instance debug information.
     *
     * @param schema the AVRO schema instance to display.
     */
    void debugSchemaNode(const avro::ValidSchema &schema) const;
};

#endif /* _LOG2KAFKA_SERIALIZER_HH_ */
