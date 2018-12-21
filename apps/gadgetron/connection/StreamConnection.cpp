
#include <memory>
#include <future>

#include "StreamConnection.h"

#include "Handlers.h"
#include "Writers.h"
#include "Loader.h"

#include "readers/Primitives.h"
#include "Reader.h"
#include "Channel.h"
#include "Context.h"


namespace {

    using namespace Gadgetron::Core;
    using namespace Gadgetron::Core::Readers;
    using namespace Gadgetron::Server::Connection;
    using namespace Gadgetron::Server::Connection::Writers;
    using namespace Gadgetron::Server::Connection::Handlers;

    class ReaderHandler : public Handler {
    public:
        ReaderHandler(std::unique_ptr<Reader> &&reader, std::shared_ptr<MessageChannel> channel)
                : reader(std::move(reader)), channel(std::move(channel)) {}

        void handle(std::istream &stream) override {
            channel->push_message(reader->read(stream));
        }

        std::unique_ptr<Reader> reader;
        std::shared_ptr<MessageChannel> channel;
    };

}

#define CONFIG_ERROR "Received second config file. Only one config allowed."
#define HEADER_ERROR "Received second ISMRMRD header. Only one allowed."

namespace Gadgetron::Server::Connection {

    std::map<uint16_t, std::unique_ptr<Connection::Handler>> StreamConnection::prepare_handlers(bool &closed) {

        std::map<uint16_t, std::unique_ptr<Handler>> handlers{};

        std::function<void()> close_callback = [&]() {
            closed = true;
            channels.input->close();
        };

        handlers[FILENAME] = std::make_unique<ErrorProducingHandler>(CONFIG_ERROR);
        handlers[CONFIG]   = std::make_unique<ErrorProducingHandler>(CONFIG_ERROR);
        handlers[HEADER]   = std::make_unique<ErrorProducingHandler>(HEADER_ERROR);

        handlers[QUERY]    = std::make_unique<QueryHandler>(channels.output);
        handlers[CLOSE]    = std::make_unique<CloseHandler>(close_callback);

        for (auto &reader : loader.readers()) {
            handlers[reader.first] = std::make_unique<ReaderHandler>(std::move(reader.second), channels.input);
        }

        return handlers;
    }

    std::vector<std::unique_ptr<Writer>> StreamConnection::prepare_writers() {
        return loader.writers();
    }

    StreamConnection::StreamConnection(std::iostream &stream, Loader &loader)
            : Connection(stream), loader(loader) {

        channels.input = std::make_shared<MessageChannel>();
        channels.output = std::make_shared<MessageChannel>();

        node = loader.stream();
    }

    void StreamConnection::process(
            std::iostream &stream,
            Context context,
            Config config,
            ErrorHandler &error_handler
    ) {
        Loader loader{error_handler, std::move(context), std::move(config)};

        StreamConnection connection(stream, loader);

        connection.start(error_handler);
        connection.node->process(connection.channels.input, connection.channels.output);
        connection.join();
    }
}
