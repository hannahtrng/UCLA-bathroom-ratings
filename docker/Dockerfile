### Build/test container ###
# Define builder stage
FROM insert-cheesy-bread:base AS builder

# Share work directory
COPY . /usr/src/project
WORKDIR /usr/src/project/build

# Build and test
RUN cmake ..
RUN make
RUN ctest --output-on-failure


### Deploy container ###
# Define deploy stage
FROM ubuntu:noble AS deploy

# Copy server output binary to "."
COPY --from=builder /usr/src/project/build/bin/server_main .
COPY --from=builder /usr/src/project/assets/ /assets/
COPY --from=builder /usr/src/project/server_config_files/example.conf .

# Expose some port(s)
EXPOSE 80

# Use ENTRYPOINT to specify the binary name
ENTRYPOINT ["./server_main"]

# Use CMD to specify arguments to ENTRYPOINT
CMD ["./example.conf"]
