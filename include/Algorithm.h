#ifndef ALGORITHM_H
#define ALGORITHM_H
class Algorithm {
public:
    virtual void offline_industry() = 0;
    virtual bool reachability_query(int source, int target) = 0;
    virtual ~Algorithm() = default;
};

#endif // ALGORIGHM_H