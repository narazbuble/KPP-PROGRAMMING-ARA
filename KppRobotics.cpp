#include <bits/stdc++.h>
using namespace std;

struct ParentInfo {
    int prev_node;
    int prev_e;
    int prev_t;
    int action; // 0 = move, 1 = charge, 2 = wait
    int edge_w;
    int wait_time; 
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, M;
    if (!(cin >> N >> M)) return 0;
    string dummy;
    getline(cin, dummy);

    map<string,int> node_to_index;
    vector<string> index_to_node;

    auto ensure_node = [&](const string &s)->int {
        auto it = node_to_index.find(s);
        if (it != node_to_index.end()) return it->second;
        int idx = index_to_node.size();
        node_to_index[s] = idx;
        index_to_node.push_back(s);
        return idx;
    };

    struct Edge { int to; int w; int o; int base; int travel_time; int energy_odd; int energy_even; };
    vector<vector<Edge>> graph;

    const int SPEED = 100; // meters per minute (assumption)
    for (int i = 0; i < M; ++i) {
        string line;
        getline(cin, line);
        if (line.size() == 0) { --i; continue; } 
        stringstream ss(line);
        string u, v;
        int w, o;
        ss >> u >> v >> w >> o;
        int ui = ensure_node(u);
        int vi = ensure_node(v);
        if ((int)graph.size() < (int)index_to_node.size()) graph.resize(index_to_node.size());
        int base = w + o;
        int travel_time = max(1, (w + SPEED - 1) / SPEED); 
        int energy_odd  = (base * 13 + 5) / 10; // round half-up
        int energy_even = (base * 8  + 5) / 10;
        graph[ui].push_back({vi, w, o, base, travel_time, energy_odd, energy_even});
        graph[vi].push_back({ui, w, o, base, travel_time, energy_odd, energy_even});
    }

    // S T line
    string st_line;
    getline(cin, st_line);
    while(st_line.size()==0) getline(cin, st_line);
    {
        stringstream ss(st_line);
        string S, T;
        ss >> S >> T;
        int si = ensure_node(S);
        int ti = ensure_node(T);
    }

    // rest points line
    string rest_line;
    getline(cin, rest_line);
    while(rest_line.size()==0) getline(cin, rest_line);
    set<int> rest_points;
    {
        if (rest_line != "-") {
            stringstream ss(rest_line);
            string token;
            while (ss >> token) {
                int idx = ensure_node(token);
                rest_points.insert(idx);
            }
        }
    }

    // charging stations
    string charge_line;
    getline(cin, charge_line);
    while(charge_line.size()==0) getline(cin, charge_line);
    set<int> charging_stations;
    {
        if (charge_line != "-") {
            stringstream ss(charge_line);
            string token;
            while (ss >> token) {
                int idx = ensure_node(token);
                charging_stations.insert(idx);
            }
        }
    }

    // node M and E 
    string nodeM_line;
    getline(cin, nodeM_line);
    while(nodeM_line.size()==0) getline(cin, nodeM_line);
    // node E
    string nodeE_line;
    getline(cin, nodeE_line);
    while(nodeE_line.size()==0) getline(cin, nodeE_line);

    // start hour
    int start_hour;
    string hour_line;
    getline(cin, hour_line);
    while(hour_line.size()==0) getline(cin, hour_line);
    {
        stringstream ss(hour_line);
        ss >> start_hour;
    }

    int n_nodes = index_to_node.size();
    if ((int)graph.size() < n_nodes) graph.resize(n_nodes);

   
    vector< map<pair<int,int>, int> > dist(n_nodes);
    vector< map<pair<int,int>, ParentInfo> > parent(n_nodes);

  
    using State = tuple<int,int,int,int>; // (total_energy, u, energy_left, t_mod120)
    priority_queue<State, vector<State>, greater<State>> pq;

    int start_idx = node_to_index[index_to_node[0]]; // dummy init find actual S index
   
    {
        stringstream ss(st_line);
        string S, T;
        ss >> S >> T;
        start_idx = node_to_index[S];
    }
    int target_idx;
    {
        stringstream ss(st_line);
        string S, T;
        ss >> S >> T;
        target_idx = node_to_index[T];
    }

    // t = 0 (minutes since start)
    dist[start_idx][{1000, 0}] = 0;
    parent[start_idx][{1000,0}] = {-1,-1,-1,-1,0,0};
    pq.push({0, start_idx, 1000, 0});

    int INF = 1e9;
    int best_energy = INF;
    pair<pair<int,int>, int> final_key;

    while(!pq.empty()) {
        auto top = pq.top(); pq.pop();
        int total_energy = get<0>(top);
        int u            = get<1>(top);
        int e_left       = get<2>(top);
        int t            = get<3>(top);
        auto it = dist[u].find({e_left, t});
        if (it == dist[u].end()) continue;
        if (it->second != total_energy) continue;

        if (u == target_idx) {
            if (total_energy < best_energy) {
                best_energy = total_energy;
                final_key = {{u, e_left}, t};
            }

            continue;
        }

        // 1
        if (charging_stations.count(u) && e_left < 1000) {
            int new_e = 1000;
            int new_t = t; 
            int new_total = total_energy; 
            // If any charging to count in objective, set new_total += (new_e - e_left);

            auto it2 = dist[u].find({new_e, new_t});
            if (it2 == dist[u].end() || new_total < it2->second) {
                dist[u][{new_e, new_t}] = new_total;
                parent[u][{new_e, new_t}] = {u, e_left, t, 1, 0, 0};
                pq.push({new_total, u, new_e, new_t});
            }
        }

        // 2
        if (rest_points.count(u)) {
            // current hour parity:
            int cur_hour = (start_hour + t/60) % 2;
            // i want to consider either no wait, or minimal wait to flip hour parity (i.e., wait minutes until floor((t+wait)/60) toggles)
            // compute minute within current hour: minute_in_hour = t % 60
            int minute_in_hour = t % 60;
            int wait_to_flip_hour = (60 - minute_in_hour) % 60; // waiting this many minutes changes floor((t+wait)/60)
            if (wait_to_flip_hour == 0) wait_to_flip_hour = 60; // if exactly at hour boundary, waiting 60 flips

           
            int new_t = (t + wait_to_flip_hour) % 120;
            int new_e = e_left;
            int new_total = total_energy; // waiting doesn't add energy
            auto itw = dist[u].find({new_e, new_t});
            if (itw == dist[u].end() || new_total < itw->second) {
                dist[u][{new_e, new_t}] = new_total;
                parent[u][{new_e, new_t}] = {u, e_left, t, 2, 0, wait_to_flip_hour};
                pq.push({new_total, u, new_e, new_t});
            }
        }

        // 3
        for (auto &ed : graph[u]) {
            int base = ed.base;
            int travel_time = ed.travel_time;
            int hour_parity = (start_hour + (t / 60)) % 2; // 1 = odd, 0 = even
            int cost = (hour_parity == 1) ? ed.energy_odd : ed.energy_even;
           
            if (e_left < cost) continue;
            int new_e = e_left - cost;
            int new_t = (t + travel_time) % 120;
            int new_total = total_energy + cost;
            int v = ed.to;

            auto itv = dist[v].find({new_e, new_t});
            if (itv == dist[v].end() || new_total < itv->second) {
                dist[v][{new_e, new_t}] = new_total;
                parent[v][{new_e, new_t}] = {u, e_left, t, 0, travel_time, 0};
                pq.push({new_total, v, new_e, new_t});
            }
        }
    }

    if (best_energy == INF) {
        cout << "Robot gagal dalam mencapai tujuan :(" << '\n';
        return 0;
    }

    // reconstruct 
    vector<int> path_nodes;
    vector<int> arrival_t;
    vector<int> energy_left_seq;
    int node = final_key.first.first;
    int eleft = final_key.first.second;
    int tmod = final_key.second;

    while (!(node == start_idx && eleft == 1000 && tmod == 0)) {
        path_nodes.push_back(node);
        arrival_t.push_back(tmod);
        energy_left_seq.push_back(eleft);
        auto pit = parent[node].find({eleft, tmod});
        if (pit == parent[node].end()) break; // safety
        ParentInfo p = pit->second;
        if (p.prev_node == -1) break;
        node = p.prev_node;
        eleft = p.prev_e;
        tmod = p.prev_t;
    }
    // start
    path_nodes.push_back(start_idx);
    arrival_t.push_back(0);
    energy_left_seq.push_back(1000);

    reverse(path_nodes.begin(), path_nodes.end());
    reverse(arrival_t.begin(), arrival_t.end());
    reverse(energy_left_seq.begin(), energy_left_seq.end());

 
    vector<int> absolute_times(path_nodes.size(), 0);
    // We'll reconstruct times by re-simulating the transitions using parent stored info:
    // Start at time 0
    absolute_times[0] = 0;
    for (size_t i = 1; i < path_nodes.size(); ++i) {
        int cur = path_nodes[i-1];
        int next = path_nodes[i];
        // find parent info at next state keyed by (energy_left_seq[i], arrival_t[i])
        auto pit = parent[next].find({energy_left_seq[i], arrival_t[i]});
        if (pit == parent[next].end()) {
            // fallback: assume travel_time 0
            absolute_times[i] = absolute_times[i-1];
        } else {
            ParentInfo pi = pit->second;
            if (pi.action == 0) {
                // move: travel time stored in edge_w
                absolute_times[i] = absolute_times[i-1] + pi.edge_w;
            } else if (pi.action == 1) {
                // charge: we assumed no time
                absolute_times[i] = absolute_times[i-1];
            } else if (pi.action == 2) {
                // wait
                absolute_times[i] = absolute_times[i-1] + pi.wait_time;
            } else {
                absolute_times[i] = absolute_times[i-1];
            }
        }
    }

    cout << "Total energi minimum: " << best_energy << '\n';
    cout << "Jalur: ";
    for (size_t i = 0; i < path_nodes.size(); ++i) {
        if (i) cout << " -> ";
        cout << index_to_node[path_nodes[i]];
    }
    cout << '\n';
    cout << "Waktu tiba:" << '\n';
    for (size_t i = 0; i < path_nodes.size(); ++i) {
        cout << index_to_node[path_nodes[i]] << " (menit " << absolute_times[i] << ")\n";
    }

    return 0;
}


