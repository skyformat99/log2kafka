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

#include "ClientProxy.hh"

using namespace std;
using namespace log4cxx;

LoggerPtr ClientProxy::logger(Logger::getLogger("ClientProxy"));

ClientProxy::ClientProxy() {
    initDefaults();
}

ClientProxy::~ClientProxy() {
    flush();
    rd_kafka_destroy(kafka_client_);
}

void ClientProxy::clientId(std::string clientId) {
    this->clientId_ = clientId;
}

void ClientProxy::messageKey(std::string messageKey) {
    this->messageKey_ = messageKey;
}

void ClientProxy::host(std::string host) {
    this->host_ = host;
}

void ClientProxy::port(int port) {
    this->port_ = port;
}

void ClientProxy::topic(std::string topic) {
    vector<string> fields;

    boost::split(fields, topic, boost::is_any_of(":"));

    if (fields.size() >= 2) {
        try {
            partition_ = boost::lexical_cast<int>(fields[1]);
        }
        catch (exception& e) {
            LOG4CXX_WARN(logger,
                    "Invalid partition value: " << fields[1] << ". Defaulting to: " << partition_);
        }
    }

    topic_ = fields[0];
}

void ClientProxy::requiredAcks(int requiredAcks) {
    this->requiredAcks_ = requiredAcks;
}

void ClientProxy::timeoutAcks(int timeoutAcks) {
    this->timeoutAcks_ = timeoutAcks;
}

void ClientProxy::serializer(const string& configFile) {
    unique_ptr<Serializer> serializer(new Serializer(configFile));
    this->serializer_ = move(serializer);
}

void ClientProxy::initDefaults() {
    clientId_ = Constants::DEFAULT_CLIENT_ID;
    requiredAcks_ = Constants::DEFAULT_REQUIRED_ACKS;
    timeoutAcks_ = Constants::DEFAULT_TIMEOUT_ACKS;
    partition_ = RD_KAFKA_PARTITION_UA;
}

void ClientProxy::connect() {

    /* Kafka configuration */

    kafka_conf_ = rd_kafka_conf_new();

    /* Set up a message delivery report callback.
     * It will be called once for each message, either on successful
     * delivery to broker, or upon failure to deliver to broker. */
    rd_kafka_conf_set_dr_cb(kafka_conf_, ClientProxy::deliverCallback);

    /* Create Kafka handle */

    char errstr[512];

    if (!(kafka_client_ = rd_kafka_new(RD_KAFKA_PRODUCER, kafka_conf_, errstr, sizeof(errstr)))) {
        throw ProducerCreationException(errstr);
    }

    /* Add brokers */

    ostringstream broker;
    broker << host_ << ":" << port_;

    if (rd_kafka_brokers_add(kafka_client_, broker.str().data()) == 0) {
        throw InvalidBrokerException();
    }
}

void ClientProxy::flush() {
    // TODO: timeout value refactoring
    rd_kafka_poll(kafka_client_, 1000);
}

void ClientProxy::sendMessage(const string& message) {

    LOG4CXX_DEBUG(logger, "Message: " << message);

    rd_kafka_topic_conf_t *kafkaTopicConfig;
    rd_kafka_topic_t *kafkaTopic;
    bool sendRawMessage = false;

    /* Prepare Kafka Topic */

    kafkaTopicConfig = rd_kafka_topic_conf_new();
    kafkaTopic = rd_kafka_topic_new(kafka_client_, topic_.data(), kafkaTopicConfig);

    /* Prepare message */

    auto_ptr<avro::OutputStream> dataOutput = avro::memoryOutputStream();

    if (message.length() == 0) {
        LOG4CXX_WARN(logger, "Empty message entry discarded");
        return;
    }

    if (serializer_) { // Use serialization mode
        LOG4CXX_DEBUG(logger, "Schema defined. Using serialization mode");

        try {
            serializer_->serialize(message, dataOutput);
        }
        catch (exception& e) {
            sendRawMessage = true;
            LOG4CXX_ERROR(logger, "Using raw mode due to unexpected exception: " << e.what());
        }
    }
    else { // Use raw mode
        sendRawMessage = true;
        LOG4CXX_DEBUG(logger, "No schema defined. Using raw mode");
    }

    /* Send request */

    // Copy message key
    size_t keyLength = messageKey_.length();
    char* key = new char[keyLength];
    memcpy(key, messageKey_.data(), keyLength);

    // Copy message value
    size_t valueLength = 0;
    uint8_t* value = NULL;

    if (sendRawMessage) {
        valueLength = message.length();
        value = new uint8_t[valueLength];

        memcpy(value, message.data(), valueLength);
    }
    else {
        valueLength = dataOutput->byteCount();
        value = new uint8_t[valueLength];

        auto_ptr<avro::InputStream> dataInput = avro::memoryInputStream(*dataOutput);
        avro::StreamReader reader(*dataInput);
        reader.readBytes(value, valueLength);
    }

    if (LOG4CXX_UNLIKELY(logger->isDebugEnabled())) {
        LOG4CXX_DEBUG(logger, "Message to be sent:");
        LOG4CXX_DEBUG(logger, "MESSAGE BEGIN");

        // The entire message is not printed with standard cout mechanism
        // due to NULL character interpretation
        cout.write(reinterpret_cast<const char*>(value), valueLength);
        cout << endl;

        LOG4CXX_DEBUG(logger, "MESSAGE END");
    }

    /* Send/Produce message. */

    rd_kafka_produce(kafkaTopic, partition_, RD_KAFKA_MSG_F_FREE, reinterpret_cast<char *>(value),
            valueLength, key, keyLength, NULL);

    LOG4CXX_INFO(logger,
            "Sent " << valueLength << " bytes to topic " << rd_kafka_topic_name(kafkaTopic) << ":" << partition_);

    /* Poll to handle delivery reports */

    rd_kafka_poll(kafka_client_, 0);

    delete key;
    //delete value; // Clean forced by RD_KAFKA_MSG_F_FREE
}

void ClientProxy::deliverCallback(rd_kafka_t *rk, void *payload, size_t len, rd_kafka_resp_err_t error_code,
        void *opaque, void *msg_opaque) {

    if (error_code) {
        LOG4CXX_WARN(logger, "Message delivery failed with error code: " << error_code);
    }
    else {
        LOG4CXX_INFO(logger, "Message delivered (" << len << " bytes): ");
    }
}

int ClientProxy::generateCorrelationId() {
    hash<std::string> hash_fn;
    time_t now = time(NULL);
    return hash_fn(ctime(&now));
}