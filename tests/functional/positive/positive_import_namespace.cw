# Verifies an imported namespace alias supports qualified nested-class construction and calls.
import network as Net

start {
    Net.Http.Client client()
    print(client.get("/"))
}
