#if 0
#include "worldcraftpro.hpp"

#include <littl/Thread.hpp>

namespace worldcraftpro
{
    class ModelPreviewImpl : public ModelPreviewCache
    {
        struct Request
        {
            int id;
            String modelFileName;
        };

        List<int> buildSizes;
        List<Request> requests;
        //List<> completed;

        Mutex buildSizesLock, queueLock;
        int next_id;

        public:
            ModelPreviewImpl() : next_id(0) {}
            virtual ~ModelPreviewImpl() {}

            // Any thread
            virtual void AddSize(int a) = 0;
            virtual int RequestModelPreview(const char* modelFileName) = 0;
            virtual void RetrieveModelPreviews(RetrieveCallback* cb) = 0;

            // Main (rendering) thread
            virtual void OnFrameDraw(int timelimitMsec) = 0;
    };

    void ModelPreviewImpl::AddSize(int a)
    {
        li_synchronized(buildSizesLock)

        buildSizes.add(a);
    }

    /*int ModelPreviewImpl::RequestModelPreview(const char* modelFileName)
    {
        // RequestModelPreview:
        // Check if need to rebuild
        // Preload model
        // Add to queue to be renderer by main thread

        // OnFrameDraw:
        // Finalize
        // Render
        // Add to queue for retrieval

        // RetrieveModelPreviews:
        // Flush to disk
        // Return
    }*/
}
#endif