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

#include "Constants.hh"

using namespace std;

const string Constants::DEFAULT_CLIENT_ID = "Log2Kafka Producer";
const string Constants::DEFAULT_SCHEMA_PATH = "/etc/log2kafka/";
const string Constants::DEFAULT_MESSAGE_KEY = "L2K"; // Simply the acronym for log2kafka

const int Constants::DEFAULT_REQUIRED_ACKS = 1;
const int Constants::DEFAULT_TIMEOUT_ACKS = 2000;
