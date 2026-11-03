/* SPDX-License-Identifier: LGPL-3.0-or-later
 * DEBUG-next-owner: disposable fixture appended to actual tutorial owner TU.
 * Synthetic owner/callback calls; never ordinary authored gameplay evidence. */
#include "gods98/sound.hpp"
static int next_owner_failures;
static void next_owner_check(FILE* log, int ok, const char* label)
{
    fprintf(log, "%s %s\n", ok ? "PASS" : "FAIL", label);
    if (!ok) ++next_owner_failures;
}
static uint32_t next_owner_message(uint32_t id, uint32_t flags)
{
    uint32_t args[2], result=0xdeadbeefu;
    args[0]=id;args[1]=flags;
    tutorial_command_invoke(16u,args,2u,&result);
    return result;
}
int pe32_nerp_owner_probe(BlockFile* cfg, int width, int height, FILE* log)
{
    uint32_t args[3],result,i,count,max_before,ack_before,gen;
    float timers[3];
    int mode=0,x,y;
    char key[256];
    const char* path;
    const char* pos;
    float bx,by;
    TextureSurface* image;
    AdvisorMessagePointerCapture capture;
    next_owner_failures=0;
    engine_mode_set_target(&mode);
    advisor_message_ui_gameplay_set(1);
    count=0;while(objective_advisor_token_text_get(count)!=NULL)++count;
    fprintf(log,"DEBUG-next-owner actual M01 tokens=%u sounds=%u\n",count,objective_advisor_sound_token_count());
    if(count!=27u)return 2;
    tutorial_scroll_runtime_state_reset();
    next_owner_check(log,tutorial_message_history_get(0u)==advisor_inline_last_sound_get(),"history0 uses actual sound owner");
    next_owner_check(log,next_owner_message(0u,0u)==0u && engine_overlay_word_peek(1)==(uint32_t)(uintptr_t)objective_advisor_token_text_get(0u),"zero ID resolves actual first token pointer");
    max_before=s_nerp_runtime.message_maximum;
    next_owner_check(log,next_owner_message(1u,0u)==1u && s_nerp_runtime.message_maximum==max_before && s_nerp_runtime.message_pending_ack==1u,"one-based first ID and duplicate history");
    mode=1;engine_mode_set_target(&mode);
    next_owner_check(log,next_owner_message(2u,0u)==0u && s_nerp_runtime.message_maximum==max_before,"mode gate preserves history");
    mode=0;engine_mode_set_target(&mode);
    for(i=2u;i<=8u;++i)next_owner_message(i,0u);
    next_owner_check(log,s_nerp_runtime.message_maximum==8u,"eight actual distinct token IDs");
    s_nerp_runtime.message_offset=2u;
    next_owner_message(9u,0u);
    next_owner_check(log,s_nerp_runtime.message_maximum==5u && s_nerp_runtime.message_offset==3u,"compaction and shared offset advance");
    for(i=1u;i<=5u;++i)next_owner_check(log,tutorial_message_history_get(i)==i+3u,"compacted real token history entry");
    ack_before=s_nerp_runtime.message_ack_count;
    s_nerp_runtime.message_offset=2u;
    tutorial_advisor_scroll_step_advance();
    next_owner_check(log,s_nerp_runtime.message_offset==1u && engine_overlay_word_peek(1)==(uint32_t)(uintptr_t)objective_advisor_token_text_get(7u),"NEXT history offset selects real history token");
    tutorial_advisor_scroll_step_advance();
    next_owner_check(log,s_nerp_runtime.message_offset==0u && s_nerp_runtime.message_ack_count==ack_before && s_nerp_runtime.message_pending_ack==1u,"return to newest does not acknowledge");
    s_nerp_runtime.message_offset=7u;
    tutorial_command_invoke(200u,NULL,0u,&result);
    next_owner_check(log,result==0u && s_nerp_runtime.message_offset==0u && s_nerp_runtime.message_ack_count==ack_before,"command200 signed-clamps same offset then NEXT");
    args[0]=1000u;args[1]=1000u;args[2]=6000u;
    tutorial_command_invoke(108u,args,3u,&result);tutorial_message_timer_values_get(timers);
    next_owner_check(log,result==1u&&timers[0]==1000.0f&&timers[1]==1000.0f&&timers[2]==6000.0f,"actual timer configuration owner");
    next_owner_message(2u,1u);
    if(objective_advisor_sound_token_count()>1u){
        int slot=(int)objective_advisor_sound_token_handle_get(1u)-1;
        float expected=(float)sound_streamed_wave_buffer_step_rate(slot)*1000.0f+1000.0f;
        next_owner_check(log,s_nerp_runtime.message_timer==expected,"actual voiced resource duration");
    }
    next_owner_message(2u,1u);
    next_owner_check(log,s_nerp_runtime.message_timer==6000.0f,"same sound fallback no-sound duration");
    tutorial_message_duration_set(5000.0f);tutorial_frame_timer_array_advance(25.0f);
    next_owner_check(log,s_nerp_runtime.message_timer==4000.0f,"25Hz timer units decay by one second");
    next_owner_message(2u,0u);tutorial_message_duration_set(5000.0f);tutorial_frame_timer_array_advance(25.0f);
    next_owner_check(log,deferred_callback_peek()!=0u&&s_nerp_runtime.message_timer==5000.0f,"configured NEXT holds real timer while pending");
    snprintf(key,sizeof(key),"%s Main NextButton%ix%i",util_root_name(cfg),width,height);path=block_get_strptr(cfg,key);
    snprintf(key,sizeof(key),"%s Main NextButtonPos%ix%i",util_root_name(cfg),width,height);pos=block_get_strptr(cfg,key);
    if(!path||!pos||sscanf(pos,"%f,%f",&bx,&by)!=2)return 2;
    image=image_surface_load(path,0u,0u);if(!image)return 2;
    x=(int)(bx+image->width*.5f);y=(int)(by+image->height*.5f);texture_release(image);
    for(i=0;i<16u;++i)advisor_message_ui_draw(0.0f);
    memset(&capture,0,sizeof(capture));ack_before=s_nerp_runtime.message_ack_count;
    advisor_message_pointer_capture_dispatch(&capture,x,y,1);
    next_owner_check(log,capture.owner==1&&s_nerp_runtime.message_ack_count==ack_before,"real NEXT image DOWN captures without ack");
    advisor_message_pointer_capture_dispatch(&capture,x,y,0);
    next_owner_check(log,s_nerp_runtime.message_pending_ack==0u&&s_nerp_runtime.message_ack_count==ack_before+1u,"real NEXT release acknowledges once");
    advisor_message_pointer_capture_dispatch(&capture,x,y,0);
    next_owner_check(log,s_nerp_runtime.message_ack_count==ack_before+1u,"duplicate UP never acknowledges twice");
    tutorial_frame_timer_array_advance(25.0f);
    next_owner_check(log,s_nerp_runtime.message_timer==0.0f,"ack releases actual timer");
    next_owner_message(2u,0u);memset(&capture,0,sizeof(capture));advisor_message_pointer_capture_dispatch(&capture,x,y,1);
    gen=objective_advisor_token_generation_get();
    result=objective_advisor_token_file_load("Levels\\TutorialLevels\\MoveTuto_01\\M01.txt");
    next_owner_check(log,result!=0u&&gen!=objective_advisor_token_generation_get(),"actual M01 reload replaces token generation");
    advisor_message_pointer_capture_dispatch(&capture,x,y,0);
    next_owner_check(log,s_nerp_runtime.message_maximum==0u&&s_nerp_runtime.message_ack_count==0u&&engine_overlay_word_peek(0)==0u,"old capture cannot retain freed text or acknowledge new tokens");
    fprintf(log,"DEBUG-next-owner failures=%d\n",next_owner_failures);
    return next_owner_failures?1:0;
}
