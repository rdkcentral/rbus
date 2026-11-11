[33mcommit d7f976c072171d57f4d916989348692d798d0134[m[33m ([m[1;36mHEAD[m[33m -> [m[1;32mfeature/mythosrbus_2[m[33m, [m[1;31morigin/feature/mythosrbus_2[m[33m)[m
Author: Aryan Y <aryan_duttay@comcast.com>
Date:   Wed Nov 11 10:51:59 2026 +0000

    Revert "1to5 fix"
    
    This reverts commit 736bd6b9f2c222f2ca95b5400885ef813a9e9b78.

[33mcommit 736bd6b9f2c222f2ca95b5400885ef813a9e9b78[m
Author: Aryan Y <aryan_duttay@comcast.com>
Date:   Wed Nov 11 10:16:20 2026 +0000

    1to5 fix

[33mcommit 1cd3abddfa23776db7dbd70afe3f2f1b306be00f[m[33m ([m[1;31morigin/develop[m[33m, [m[1;31morigin/HEAD[m[33m, [m[1;31morigin/Coverity_rbus[m[33m, [m[1;32mdevelop[m[33m)[m
Author: ap934 <Akhil_P@comcast.com>
Date:   Wed Oct 21 16:59:54 2026 +0530

    RDKCOM-5665: RDKBDEV-3520,RDKBACCL-2161 rbuscli fails to build with “too many arguments” on the session commands (#461)
    
    Reason for change: validate_and_execute_create_session_cmd() and validate_and_execute_get_session_cmd() are called as (argc, argv) but declared (). Under C23 (GCC 15) that is an error, declare them to match
    Test Procedure: bitbake rdk-generic-broadband-image and the dependent packages
    Risk : None
    
    Signed-off-by: ap934 <Akhil_P@comcast.com>

[33mcommit e8cf808f165fafc6b8e03184ecd8d5484cbf37b2[m[33m ([m[1;31morigin/topic/mutex[m[33m)[m
Author: Chris Buchter <100870495+cbucht200@users.noreply.github.com>
Date:   Fri Oct 9 14:45:35 2026 -0400

    RDKCOM-5684: Add user_data API to RBus (#459)
    
    Reason for change: Rbus users need the ability to attach user_data to rbus_handle_t
    Test Procedure: Automated tests
    Risks: Low
    Priority: P2
    Signed-off-by: Chris_Buchter@cable.comcast.com

[33mcommit d9f40e18798e3b1f6ce6394fc431bc4499f15da9[m
Author: Chris Buchter <100870495+cbucht200@users.noreply.github.com>
Date:   Thu Oct 8 10:48:02 2026 -0400

    chore: add devcontainer support for rbus development in containerized environment (#458)
    
    * chore: add devcontainer support for rbus development in containerized environment
    
    * Potential fix for pull request finding 'Use dynamic workspace path for non-rbus checkout names'
    
    Co-authored-by: Copilot Autofix powered by AI <175728472+Copilot@users.noreply.github.com>
    
    * fix: pin linenoise to tag 2.0, fix header size in README
    
    * fix: README needed install prefix
    
    ---------
    
    Co-authored-by: Copilot Autofix powered by AI <175728472+Copilot@users.noreply.github.com>

[33mcommit 5f74c2fbe0019781410be4b1160768adda96d1a4[m[33m ([m[1;33mtag: [m[1;33mv2.14.0[m[33m, [m[1;31morigin/release[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Wed Sep 9 19:47:24 2026 +0000

    Release of v2.14.0
    
    Signed-off-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit 9b3d72f9bd9ee440f02497c1e72398694c03e21f[m
Author: tabbas651 <74683978+tabbas651@users.noreply.github.com>
Date:   Wed Sep 9 15:40:10 2026 -0400

    RDK-61800: Enable Cross-Path Trace Context Propagation (#454)
    
    Reason for change: Implemented the event trace propogation for event Consumer /Provider.
    Risks: Medium
    Priority: P0
    Signed-off-by: Thamim Razith Abbas Ali tabbas651@comcast.com

[33mcommit 0906a474b2b3d60c09d1fa71eabaede8929f3cb7[m
Author: gourivarma3 <gouri_varma@comcast.com>
Date:   Mon Aug 10 18:41:31 2026 +0530

    RDKDEV-1444, RDKBDEV-3389 Add Documentation RBus (#450)
    
    * Added README.md for RBus
    
    * Apply suggestions from code review
    
    Co-authored-by: Copilot Autofix powered by AI <175728472+Copilot@users.noreply.github.com>
    
    * Apply suggestions from code review
    
    Co-authored-by: Copilot Autofix powered by AI <175728472+Copilot@users.noreply.github.com>
    
    * Potential fix for pull request finding
    
    Co-authored-by: Copilot Autofix powered by AI <175728472+Copilot@users.noreply.github.com>
    
    * Potential fix for pull request finding
    
    Co-authored-by: Copilot Autofix powered by AI <175728472+Copilot@users.noreply.github.com>
    
    * addressed internal review comment
    
    * Removed Initialization Call Flow section
    
    Removed the Initialization Call Flow diagram from the README.
    
    ---------
    
    Co-authored-by: Copilot Autofix powered by AI <175728472+Copilot@users.noreply.github.com>

[33mcommit 3ee1b58fd8d1f71b0fa824e55bb5182276f6e59c[m[33m ([m[1;33mtag: [m[1;33mv2.13.0[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Wed Jul 22 00:22:14 2026 +0000

    Release of v2.13.0
    
    Signed-off-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit 72d4b7a825fdc14cf016165c5f3351efd353824c[m
Author: Petro Krynytskyi <petr0krynytskiy@gmail.com>
Date:   Wed Jun 17 21:53:32 2026 +0300

    XB10-2715: Avoid redundant tableSyncHandler calls during wildcard get resolution (#449)
    
    Reason for change: During wildcard get resolution, the rbus implementation may call the tableSyncHandler multiple times for the same parameter,
                       which can lead to unnecessary overhead, potential performance issues or even memory corruption.
    Test Procedure: Tested and verified
    Priority: P1
    Risks: Low
    
    Signed-off-by: Petro Krynytskyi <33368124+WellPeter@users.noreply.github.com>

[33mcommit 3e344546469f2ac2daf4314ff6acc7ca718580f6[m[33m ([m[1;33mtag: [m[1;33mv2.12.0[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Tue Mar 3 16:07:46 2026 +0000

    Release of v2.12.0
    
    Signed-off-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit 4a25e92112e827f7007de52488666c81c4564b5a[m[33m ([m[1;31morigin/topic/rttimeraw[m[33m)[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Mon Feb 23 21:02:03 2026 +0530

    Update code for better readability (#443)
    
    * Update rtrouted.c
    
    * Update rtrouteBase.c

[33mcommit d619aa6e8aaf70bf6a8c02ed772e4d46073bab2b[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Mon Feb 23 03:43:45 2026 +0530

    RDK-60607: Fix for y2k38 issue and integer overflow (#442)
    
    * Fix for y2k38 issue and integer overflow
    * Address Copilot review comments
    * Change from uint64_t to size_t
    
    Signed-off-by: dshett549 <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    Co-authored-by: Copilot <175728472+Copilot@users.noreply.github.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 9c7c63688654ded108bedaeb13559b081c79bb35[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Sun Feb 22 23:50:34 2026 +0530

    RDK-60607: Data race condition,rtSemaphore_TimedWait (#439)
    
    * RDK-60607: (cid:559612) Data race condition,rtSemaphore_TimedWait
    
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
    Co-authored-by: Copilot <175728472+Copilot@users.noreply.github.com>

[33mcommit aafdf2d28ff41c8b332e86d52cfc27b3410f67d5[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Wed Feb 18 20:30:29 2026 +0530

    RDK-60607: (cid:559883) Data race condition, rtConnection_CallbackThread (#440)
    
    * RDK-60607: (cid:559883) Data race condition, rtConnection_CallbackThread

[33mcommit cdad3e60333bd4b660d910966d48163c294ed81b[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Wed Feb 18 00:52:27 2026 +0530

    RDK-60607: (cid:278291)  Unchecked return value, rtThreadPool_StopAll… (#436)
    
    * RDK-60607: (cid:278291)  Unchecked return value, rtThreadPool_StopAllThread
    
    Signed-off-by: dshett549 <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit bd74c91b0adb6ac59f98bc07679e4ac7d6abf322[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Tue Feb 17 20:20:44 2026 +0530

    RDK-60607: Coverity fix in rtmessage (#434)
    
    * RDK-60607: (cid:259464) Resource leak rtConnection_CreateInternal
    
    Signed-off-by: dshett549 <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit f18a0c72806eaa05ea39af95c4825d7aa39de360[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Mon Feb 9 20:12:30 2026 +0530

    RDK-60409: Component independent Tool Development for Noisy Log detection (#433)
    
    RDK-60409: Component independent Tool Development for Noisy Log detection (#433)
    
    
    Co-authored-by: Copilot <175728472+Copilot@users.noreply.github.com>
    Signed-off-by:  Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit d3530450b54c1cccdf0b9b08225f3c14101162d8[m
Author: Permanence AI Coder <166843908+Permanence-AI-Coder@users.noreply.github.com>
Date:   Sun Dec 28 22:27:15 2025 -0500

    rtmessage: add null checks for arguments in rtMessageHeader_Decode (#383)
    
    The changes in this PR were automatically generated by the Permanence AI
    Coder and reviewed by @jweese and @dnovick. This change adds null checks
    for the `hdr` and `buff` arguments at the start of
    `rtMessageHeader_Decode`. It also removes redundant null checks for
    `ptr` later in the function.
    
    on-behalf-of: @permanence-ai <github-ai@permanence.ai>

[33mcommit 45f5d1e40d2596b50634f89a2037aaaedb1291d0[m[33m ([m[1;33mtag: [m[1;33mv2.11.0[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Tue Dec 23 21:04:27 2025 +0000

    Release of v2.11.0
    
    Signed-off-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit a1b99efa895a1acfe53a191d78c3ee343572a91c[m
Author: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
Date:   Tue Dec 23 15:29:27 2025 -0500

    RDKB-59800 : Handle the subscription of event upon crash recovery (#429)
    
    Reason for change: Upon crash n recovery of the provider, the event subscription resumed. But when Consumer re-subscribes, it should handle it gracefully
    Test Procedure: as per RDKB-59800
    Risks: Medium
    
    Signed-off-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit 64c33adf1df23153929289b0118ac53a97c854dc[m
Author: Permanence AI Coder <166843908+Permanence-AI-Coder@users.noreply.github.com>
Date:   Mon Dec 8 22:27:35 2025 -0500

    rbuscli: fixup unreachable code in hints function (#388)
    
    The changes in this PR were automatically generated by the Permanence AI
    Coder and reviewed by @jweese.
    
    The condition else if (num == 3) is repeated twice, making the second
    block unreachable. Since the intent of the conditions is to switch on
    the number of arguments counted, this change updates the second
    condition to compare num to 4. The expected value of hint in that block
    is consistent with num being 4 as opposed to 3.
    
    on-behalf-of: @permanence-ai <github-ai@permanence.ai>
    
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 35612bd51a44a59aab8b69a0b5034a867b522ded[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Tue Dec 9 02:34:16 2025 +0530

    RDKB-62669 : rbus build failure with TEST DISTRO (#390)
    
    * RDKB-62669 : rbus build failure with TEST DISTRO
    
    Reason for change: rbus build failure with TEST DISTRO
    Test Procedure: Tested & verified
    Risks: Medium
    Signed-off-by: Rose Mary Benny <RoseMary_Benny@comcast.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit becfd1a4ca3c3d95594949530990c66c8dfecc65[m
Author: Permanence AI Coder <166843908+Permanence-AI-Coder@users.noreply.github.com>
Date:   Sun Dec 7 22:16:17 2025 -0500

    rbus: prevent overflow when reading timeouts (#375)
    
    The changes in this PR were automatically generated by the Permanence AI
    Coder and reviewed by @jweese and @dnovick.
    
    In each of the `rbusHandle_Fetch*Timeout` functions, there is a
    possibility that the integer read from the file is too large, causing an
    overflow and incorrect result for the timeout. This change introduces a
    helper function `rbusHandle_FetchTimeoutFromFile` and calls it from each
    of the four functions. As an enhancement, the helper function uses a
    float instead of the old implementation's integer to allow fractional
    timeouts. In the case that no data was read, that the float was too big
    (`ERANGE`), that the float was negative, or that multiplication would
    cause an overflow, the helper function returns a provided default value.
    
    on-behalf-of: @permanence-ai <github-ai@permanence.ai>

[33mcommit 20ca04b064a02a8a17bed49d2be2e58446a69006[m
Author: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
Date:   Tue Dec 2 15:47:19 2025 -0500

    Revert "RDKB-62423 : Use CLOCK_MONOTONIC_RAW across rbus (#379)" (#416)
    
    This reverts commit 6de9e6d5e4a0c89902d4b08f2f02500d2fcf8300.

[33mcommit 75fb1a2b39d43f1335f42ba7a3ee0fb84ba1cb26[m
Author: Permanence AI Coder <166843908+Permanence-AI-Coder@users.noreply.github.com>
Date:   Tue Dec 2 11:26:34 2025 -0500

    rbus: fixup unreachable code in rbus_open (#387)
    
    The changes in this PR were automatically generated by the Permanence AI
    Coder and reviewed by @dnovick and @jweese.
    
    In the original code, the failure cases of both rbus_registerObj and
    rbusHandle_TimeoutValuesInit use goto exit_error2. Combined with the
    return on L3047, this renders the call to rbus_unregisterObj
    unreachable. If rbus_registerObj succeeds but
    rbusHanlde_TimeoutValuesInit fails, the object is never unregistered. It
    seems the intent should be to unregister componentName if registration
    succeeds but a later operation fails. This change adds another label
    right before the unreachable rbus_unregisterObj call and jumps there in
    the case that rbusHandle_TimeoutValuesInit fails. All other behavior is
    unaffected: if registration fails we jump to exit_error2 as before; if
    both calls succeed, we continue onto the success path.
    
    on-behalf-of: @permanence-ai <github-ai@permanence.ai>

[33mcommit 7f1b5cda990516dcb52643a6171b17a71adf4211[m
Author: Permanence AI Coder <166843908+Permanence-AI-Coder@users.noreply.github.com>
Date:   Tue Nov 25 23:25:18 2025 -0500

    rbus_message: ensure lock release in RemoveAllListeners (#377)
    
    The changes in this PR were automatically generated by the Permanence AI
    Coder and reviewed by @dnovick and @jweese.
    
    The `VERIFY_NULL` macro causes an early return; if that happens, `gMutex`
    is still held since the unlock at the end of the function is missed.
    Instead, this code logs a warning on a NULL `ctx` but continues the loop,
    ensuring the rest of the listeners are still removed and the lock is
    released at the end of the function.
    
    on-behalf-of: @permanence-ai <github-ai@permanence.ai>

[33mcommit e3ba3765affcf2a3ec1660531e65e3ffc57a631a[m
Author: Permanence AI Coder <166843908+Permanence-AI-Coder@users.noreply.github.com>
Date:   Tue Nov 25 22:54:57 2025 -0500

    rbus: check return value of remove (#376)
    
    The changes in this PR were automatically generated by the Permanence AI
    Coder and reviewed by @dnovick and @jweese.  This change checks the
    return value of `remove(filename)` and logs an error message if the removal
    fails, unless the error is `ENOENT`, which is harmless.
    
    on-behalf-of: @permanence-ai <github-ai@permanence.ai>

[33mcommit 2cfe917a67505f4da1980860d8048bdc5d7d4163[m
Author: Permanence AI Coder <166843908+Permanence-AI-Coder@users.noreply.github.com>
Date:   Wed Nov 19 13:19:13 2025 -0500

    rbus: fixup null pointer dereference in _subscribe_async_callback_handler (#382)
    
    The changes in this PR were automatically generated by the Permanence AI
    Coder and reviewed by @dnovick and @jweese. The code was checking to make sure
    `subscription` is non-NULL after it had already been dereferenced to get
    its `asyncHandler` and `handle`. This change moves the use of `subscription`
    inside the NULL check. An error message is logged in case `subscription`
    is NULL.
    
    on-behalf-of: @permanence-ai <github-ai@permanence.ai>

[33mcommit a0ccfbffbefc481f857ab25a26e243abbf08257b[m
Author: Permanence AI Coder <166843908+Permanence-AI-Coder@users.noreply.github.com>
Date:   Tue Nov 18 18:52:29 2025 -0500

    rtrouted: check mkdir return value (#374)
    
    The changes in this PR were automatically generated by the Permanence AI
    Coder and reviewed by @eelenberg and @jweese. This change adds error checking
    for the `mkdir` call, logging a warning if the directory creation fails
    (except for the case where the directory already exists).
    
    This PR complies with RDK LLM usage requirements.
    
    on-behalf-of: @permanence-ai <github-ai@permanence.ai>

[33mcommit 1d5fa68d8a8845ba9c155dafb165e67385adb8be[m
Author: Permanence AI Coder <166843908+Permanence-AI-Coder@users.noreply.github.com>
Date:   Tue Nov 18 11:19:53 2025 -0500

    rbus: check for null pointer in ValueChangeRecord (#372)
    
    The changes in this PR were automatically generated by the Permanence AI
    Coder and reviewed by @fwph. This adds a check for null pointers before
    dereferencing `rec->node` and `rec->node->cbTable.getHandler` to prevent
    potential null pointer dereferences. If either is NULL, an error message
    is logged with index information and the iteration is skipped.
    
    on-behalf-of: @permanence-ai <github-ai@permanence.ai>

[33mcommit 3e841fc46938ff3153eba29a82061526bd795bec[m
Author: Permanence AI Coder <166843908+Permanence-AI-Coder@users.noreply.github.com>
Date:   Tue Nov 18 10:59:46 2025 -0500

    rtrouted: fixup null dereference in ReadTextFile (#371)
    
    The changes in this PR were automatically generated by the Permanence AI
    Coder and reviewed by @fwph. This change adds an `else` branch to ensure
    that `(*content)[sz] = 0;` is only executed when `*content` is not null.
    
    on-behalf-of: @permanence-ai <github-ai@permanence.ai>

[33mcommit 4acdada265568b649b2e467195c4cf690ebc1d63[m
Author: Permanence AI Coder <166843908+Permanence-AI-Coder@users.noreply.github.com>
Date:   Tue Nov 18 10:58:54 2025 -0500

    rbus: fixup null dereference in SubscribeRawData (#370)
    
    The changes in this PR were automatically generated by the Permanence AI
    Coder and reviewed by @eelenberg and @fwph. This change adds checks to
    ensure `subInternal` is not NULL before dereferencing it when calling
    `rbusMessage_AddPrivateListener` and `rbusMessage_AddListener`. If
    `subInternal` is NULL, an error is logged and a suitable error code
    is returned.
    
    on-behalf-of: @permanence-ai <github-ai@permanence.ai>

[33mcommit 2e65c3bb8c06e17ae6b2acb5f97da1d52cca6ed8[m
Author: Permanence AI Coder <166843908+Permanence-AI-Coder@users.noreply.github.com>
Date:   Tue Nov 18 10:19:07 2025 -0500

    rtHashMap: fixup divide by zero in Hash_Func_String (#369)
    
    The changes in this PR were automatically generated by the Permanence AI
    Coder and reviewed by @fwph. This change adds a check to prevent a
    divide-by-zero error in the `rtHashMap_Hash_Func_String` function. If
    `buckets_size` is zero, the function now returns zero instead of attempting
    the division.
    
    on-behalf-of: @permanence-ai <github-ai@permanence.ai>

[33mcommit 976d570993fc0e7abd2422872cd0dd8b8e838b1e[m[33m ([m[1;33mtag: [m[1;33mv2.10.0[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Thu Nov 13 21:19:24 2025 +0000

    Release of v2.10.0
    
    Signed-off-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit f70280b3f44b012ac8194cc8ad53a49dd22d47be[m
Author: Jacob Gladish <jacobgladish@yahoo.com>
Date:   Thu Nov 13 12:23:53 2025 -0500

    Fixed race condition in rtRetainable reference counting (#380)
    
    * Fixed race condition in rtRetainable_releaseInternal
    
    There was a race in rtRetainable_releaseInternal where two threads
    could decrement the refcount and both call the destructor
    
    * Apply suggestion from @Copilot
    
    Co-authored-by: Copilot <175728472+Copilot@users.noreply.github.com>
    
    * Apply suggestion from @Copilot
    
    Co-authored-by: Copilot <175728472+Copilot@users.noreply.github.com>
    
    ---------
    
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
    Co-authored-by: Copilot <175728472+Copilot@users.noreply.github.com>

[33mcommit 6de9e6d5e4a0c89902d4b08f2f02500d2fcf8300[m
Author: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
Date:   Wed Nov 12 11:03:09 2025 -0500

    RDKB-62423 : Use CLOCK_MONOTONIC_RAW across rbus (#379)
    
    Reason for change: Use CLOCK_MONOTONIC_RAW across rbus
    Test Procedure: Verify the system end to end
    Risks: Medium
    
    Signed-off-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit 28a52ab99db08640ac15bcb849f3c865aefc495d[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Sat Nov 8 03:11:51 2025 +0530

    RDKB-62297: Unchecked return value from library rtrouted.c (#373)
    
    Reason for change: Unchecked return value from library rtrouted.c
    Test Procedure: Tested and verified
    Priority: P1
    Risks: Medium
    
    Signed-off-by: Rose Mary Benny <RoseMary_Benny@comcast.com>
    Signed-off-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit 7f763353fb1c4576b1c6a261a5dfeed7aeadbd39[m
Merge: 2f82b00 c7e435b
Author: Stephen Barrett <sbarre01@users.noreply.github.com>
Date:   Tue Sep 30 11:54:26 2025 +0100

    Merge pull request #367 from rdkcentral/feature/actions/develop-cla
    
    Deploy cla action

[33mcommit c7e435bc50799eee67a702ec07042894fc37ed50[m
Merge: 7dfd0bd 2f82b00
Author: Stephen Barrett <sbarre01@users.noreply.github.com>
Date:   Mon Sep 29 16:18:32 2025 +0100

    Merge branch 'develop' into feature/actions/develop-cla

[33mcommit 2f82b00b264673a42a9c878a6d10b82a37cbdfab[m[33m ([m[1;33mtag: [m[1;33mv2.9.0[m[33m)[m
Author: kamirt573_comcast <karunakaran_amirthalingam@cable.comcast.com>
Date:   Sun Sep 28 21:51:04 2025 +0000

    Release of v2.9.0
    
    Release of v2.9.0
    Signed-off-by: kamirt573_comcast <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit d8f594d286392eddc22ce68941af87cd42d8618e[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Fri Sep 26 23:06:51 2025 +0530

    RDKB-58045: Unused value in rbus.c (#366)
    
    Reason for change: log the failures
    Test Procedure: Tested and verified
    Priority: P1
    Risks: Medium
    Signed-off-by: Rose Mary Benny <RoseMary_Benny@comcast.com>

[33mcommit 7dfd0bd90ce898494d4da1c1bef848fb4461f8b6[m
Author: rdkcmf <github@code.rdkcentral.com>
Date:   Thu Sep 25 18:31:46 2025 +0100

    Deploy cla action

[33mcommit 31192d59fc70694bfb95b5e67c7ddd3d240628f5[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Mon Sep 22 06:44:44 2025 +0530

    RDK-58045 : Addressing the Big parameter passed by value in rtrouted.c (#357)
    
    Reason for change:  If rtMessageHeader is a large struct, passing it by value will copy the entire struct
                        onto the stack, which can be inefficient and unnecessary if the function does not need to modify
                        the original struct.
    Test Procedure: As per RDK-58045
    Risks: Low

[33mcommit 09873848a0ad86f28a38e6f90668a779148ea53a[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Mon Sep 22 06:38:03 2025 +0530

        RDKB-61705:setsockopt(TCP_NODELAY) failed: Operation not supported warn (#365)
    
    Reason for change: setsockopt(TCP_NODELAY) failed: Operation not supported warn observed when executed disca in rbuscli interface.
    Test Procedure: Tested and verified
    Priority: P1
    Risks: Low
    Signed-off-by: Rose Mary Benny <RoseMary_Benny@comcast.com>

[33mcommit effa0275492853d276afc2f90862eb9fb401c713[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Fri Sep 19 18:01:04 2025 +0530

    RDK-58045 : Addressing the time of check bug (#354)
    
    Reason for change: Accessing the file using stat() and access() before fopen() in rbuscore.c rbus_handle.c rbus_subscription.c
    Test Procedure: as per RDK-58045
    Risks: Low

[33mcommit 7abb3952797671b445465184607c11e828c581a1[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Wed Sep 17 07:16:49 2025 +0530

    RDKB-61485:Reset OutPointers to NULL when RBUS Discover APIs fails (#360)
    
    Reason for change: To avoid memory leak
    Test Procedure: Tested and verified
    Priority: P1
    Risks: Medium
    
    Signed-off-by: dshett549 <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 25c411b574b696361460cb19f2af7481e295ab14[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Wed Sep 17 07:06:57 2025 +0530

    RDK-58045 : Addressing the 'Constant' variable guards dead code (#359)
    
    Reason for change: local variable ret is assigned once . make it constant throughout its scope
    Test Procedure: as per RDK-58045
    Risks: Medium

[33mcommit 964dfbd9779b3b235116330fe8881d72de91539b[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Wed Sep 17 02:47:33 2025 +0530

    RDK-58045 : Addressing the dead default in switch (#358)
    
    Reason for change: unsigned int last_group_len = in_size % 3 can only range from 0,1,2
    Test Procedure: as per RDK-58045
    Risks: Medium

[33mcommit 374c853467c4aec363e14d266ae34b8a7c37e0bf[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Wed Sep 17 02:40:17 2025 +0530

    RDK-58045 : Addressing the calling risky function bug (#356)
    
    Reason for change: Calling risky function in createTreeTopic function and rtLogPrintf
    Test Procedure: as per RDK-58045
    Risks: Low

[33mcommit ae8c7bd668166a561b33a494a5cce03d654d9b18[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Wed Sep 17 02:35:29 2025 +0530

    RDK-58045 : Addressing the parse error in rbus.c (#355)
    
    Reason for change: variable prop is agin used locally in rbusPropertyList_initFromMessage function causing parse error
    Test Procedure: as per RDK-58045
    Risks: Low

[33mcommit ccec2ebd7c5be3795d4cc00d050ca27dc1423670[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Mon Sep 15 18:48:01 2025 +0530

    RDKB-61620 : Update Logging level in rbuscore (#353)
    
    * RDKB-61620: Update Logging level in rbuscore
    
    Reason for change: To avoid unwanted prints in device
    Test Procedure: Tested and verified
    Priority: P1
    Risks: Medium
    Signed-off-by: dshett549 <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 9db50b7a37c07dc7dc50b1be324b078e1abc15c1[m[33m ([m[1;33mtag: [m[1;33mv2.8.0[m[33m)[m
Author: kamirt573_comcast <karunakaran_amirthalingam@cable.comcast.com>
Date:   Wed Sep 3 15:10:40 2025 +0000

    Release of v2.8.0
    
    - Fixed medium priority coverity issues
    Signed-off-by: kamirt573_comcast <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit 739d535150feba90476f93f07bee1c91d8d77e37[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Tue Sep 2 19:22:00 2025 +0530

    RDKB-60980 : Addressing the possible issues reported in rtSemaphore.c (#347)
    
    Reason for change: Data race condition in rtSemaphore.c
    Test Procedure: as per RDKB-60980
    Risks: Medium
    
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit a054e48f869aa34453df246af5e0f3a74d7460a2[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Fri Aug 29 02:50:16 2025 +0530

    RDKB-60980 : Addressing the possible issues reported in rtConnecion.c (#351)
    
    Reason for change: Unchecked return value from library in rtConnection_ConnectAndRegister function
    Test Procedure: as per RDKB-60980
    Risks: Medium

[33mcommit c243f5e28908ab7d19cab8ae38410390fc69c77f[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Tue Aug 26 07:36:51 2025 +0530

    RDKB-60980 : Addressing issues in rtrouteBase.c (#345)
    
    * RDKB-60980 : Addressing issues in rtrouteBase.c
    
    Reason for change: Bug fix for Unchecked return value from library & dereference after null check in rtrouteBase.c file
    Test Procedure: as per RDKB-60980
    Risks: Medium

[33mcommit df9523d88ae8cca8d885dbdb9b43a5c455bff0ee[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Sat Aug 23 01:20:53 2025 +0530

    RDKB-60980 : Addressing the possible issues reported (#344)
    
    * RDKB-60980 : Addressing the possible issues reported
    
    Reason for change: Ignoring number of bytes read in rbus_handle.c
    Test Procedure: as per RDKB-60980
    Risks: Medium
    
    * RDKB-60980 : Addressing the possible issues reported
    
    Reason for change: Operands don't affect result in rbusValue_SetFromString function
    Test Procedure: as per RDKB-60980
    Risks: Medium

[33mcommit da61c6d895e43458044db447da98ad9e47e6ddcf[m
Author: rdkcmf <github@code.rdkcentral.com>
Date:   Sat Aug 9 17:34:30 2025 +0100

    Deploy cla action (#289)
    
    Co-authored-by: Stephen Barrett <sbarre01@users.noreply.github.com>

[33mcommit b6cec14d921a7ad5926f3c534712937e10089b40[m
Author: Stephen Barrett <sbarre01@users.noreply.github.com>
Date:   Sat Aug 9 17:34:20 2025 +0100

    Create CODEOWNERS file (#340)

[33mcommit 4cc34e9c8190369d05bf9b6ecbf5449f047654ee[m[33m ([m[1;33mtag: [m[1;33mv2.7.0[m[33m)[m
Author: kamirt573_comcast <karunakaran_amirthalingam@cable.comcast.com>
Date:   Wed Aug 6 23:00:13 2025 +0000

    Release of v2.7.0
    
    Signed-off-by: kamirt573_comcast <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit 8e848bc11512ed3b315d4b270de0b8a1ecd1c01b[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Tue Aug 5 22:01:00 2025 +0530

    Replacement of strncpy in test folder (#339)

[33mcommit b4bdc5282dfecbd73ed1572817a168bd468f8efc[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Mon Aug 4 17:38:31 2025 +0530

        RDKEMW-6585 - Incorrect printf Format Specifier for struct timeval tv_usec Field (#338)
    
    Reason for Change: -  rbusSampleConsumer failing

[33mcommit 93e019136b6c1a4615933bcfb5938af011ecbd39[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Sat Aug 2 01:47:59 2025 +0530

    RDKEMW-5996 - Replcement of strncpy and strcpy with rtString_Copy (#337)
    
    * RDKEMW-5996 - Replcement of strncpy and strcpy with rtString_Copy
    
    Reason for Change: -  Addressed the coverity issues
    
    * Updated based on Review Comment & Updated the send/recv buffer size
    
    ---------
    
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit a9a66c964cf8374b16e59732f63ffe3beb8e12b4[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Thu Jul 31 08:39:35 2025 -0400

    RDKEMW-5996 - Addressing Possible memory leaks reported (#335)
    
    Reason for Change: -  Addressed the coverity issues
    
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 8bd2bd90ab69f8933220d966a04ddfc657d8fd08[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Fri Jul 25 20:24:02 2025 +0530

    RDKMEW-5996 : Handle String Buffers and other corner cases of failure (#332)
    
    Reason for change: Handle String Buffers and other corner cases of failure
    Test Procedure: Tested and verified
    Signed-off-by: RoseMary_Benny@comcast.com
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 50a4a21bca422ec745e1f171ec771439b5894f5d[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Wed Jul 23 20:55:39 2025 +0530

    Updated the valgrind yml file (#333)

[33mcommit 3bdb1579902397324bb310719dedeea8969b4045[m
Author: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
Date:   Tue Jul 22 09:07:44 2025 -0400

    RDKEMW-5996 : Handle potential memory leaks (#330)
    
    Signed-off-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit 98cce95b6997c886542384df92e6ee1e4cbeb5f3[m
Author: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
Date:   Thu Jul 17 19:06:51 2025 -0400

    RDKEMW-5996 : Handle the alloc failure in rtMessage_Create (#329)
    
    Reason for change: Handle the alloc failure in rtMessage_Create
    Test Procedure: as per RDKEMW-5996
    Risks: Medium
    
    Signed-off-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit a9aef66cc3068478bf4aa71617650ee2f32ac3c7[m
Author: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
Date:   Thu Jul 17 16:43:07 2025 -0400

    RDKEMW-5996 : Handle the json parsing error (#328)
    
    Reason for change: Handle the json parsing error
    Test Procedure: pass invalid config n test
    Risks: Low
    
    Signed-off-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit 825ed139c972cd68fc7bdaaa144655c0423fcaf2[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Wed Jul 16 09:52:47 2025 -0400

    RDKB-60555 : Update the workflow (#326)
    
    Reason for change: Update the workflow to build for all the branches
    Test Procedure: Verify Eventing
    Risks: Medium
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 7f439049f53d80dde27ce4f59866abee906815ca[m
Author: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
Date:   Wed Jul 16 09:24:28 2025 -0400

    RDKB-60555 : Move the subscription mutex after notify (#327)
    
    Reason for change:
    - The rbus library maintains list of active subscription that exist for any given DML.
    - The rbus library uses mutex protection to access this list while adding, removing and while publishing.
    - The flow in rbus library is, it locks the mutex when any consumer calls Subscribe/Unsubscribe and also informs the provider about the action if callback registered and adds to the list then unlocks the mutex.
    - It is absolutely necessary to lock the mutex before we add/remove from the list but its not necessary to lock before notifying the provider about the action.
    Test Procedure: Verify Eventing
    Risks: Medium
    
    Signed-off-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit 938d56487d0399b8b075833396e2b8aee5d52ab7[m[33m ([m[1;33mtag: [m[1;33mv2.6.0[m[33m)[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Mon Jul 14 10:55:34 2025 -0400

    Release of v2.6.0 (#323)
    
    Release of v2.6.0

[33mcommit 62f31674ca4757a6cca875623ccfd8d027bd5908[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Wed Jul 9 22:59:38 2025 +0530

    RDKEMW-5321 rbus - Publish address sanitizer results as a part of CI … (#285)
    
    * RDKEMW-5321 rbus - Publish address sanitizer results as a part of CI WorkFlow
    
        Reason for change: Added the address sanitizer in workflow
        Test Procedure: Tested and verified
        Signed-off-by: RoseMary_Benny@comcast.com

[33mcommit 791c4dbd6c63b433344e0fe7763f52176d18659e[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Wed Jul 2 21:19:53 2025 +0530

    RDKB-60328: Add sampleApps in workflow to test Dropping messages when provider not responding (#293)
    
    * RDKB-60328: Add sampleApps in workflow to test Dropping messages when provider not responding
    
    Reason for change: To increase code coverage
    Test Procedure: Tested and verified
    Priority: P1
    Risks: Medium
    
    Signed-off-by: dshett549 <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    
    * Exit executables and to continue running other errors when on failure
    
    Signed-off-by: dshett549 <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    
    * Add changes to exit sampleapps and update workflow
    
    Signed-off-by: dshett549 <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    
    * Addressed review comments
    
    ---------
    
    Signed-off-by: dshett549 <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit af6d05c9bda84001fb7f2a0db56846fe76e28329[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Wed Jul 2 06:53:27 2025 +0530

    RDKEMW-5628: Implement setLogTypeFromEnvironment which reads RT_LOG_TYPE env variable (#305)
    
    * RDKEMW-5628: Implement setLogTypeFromEnvironment which reads RT_LOG_TYPE env variable
    
    Reason for change: To have option to set log type
    Test Procedure: Tested and verified
    Priority: P1
    Risks: Medium
    Signed-off-by: dshett549 <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 7c39418f38537cb35d077dccf939422645d7f081[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Tue Jul 1 19:49:46 2025 +0530

    RDKB-60344: Remove extra newline added for rbus logs in device (#294)
    
    * RDKB-60344: Remove extra newline added for rbus logs in device
    
    Reason for change: To avoid extra newline in logfile
    Test Procedure: Tested and verified
    Priority: P1
    Risks: Medium
    
    Signed-off-by: dshett549 <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit efb62ae0f7bf4e18a1447a8fb55b42818c3020c7[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Wed Jun 25 04:20:44 2025 +0530

    RDKB-60279: Remove rbus_gtest patch from rbus recipe (#288)
    
    Reason for change: Remove rbus_gtest patch from rbus recipe
    Test Procedure: Tested and verified
    Priority: P1
    Risks: High
    
    Signed-off-by: dshett549 <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit d94efc064688a3ac5a68bf77406fe8958a642f39[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Tue Jun 24 16:15:12 2025 -0400

    RDKEMW-5066 : Enabling the all the sample apps to install target (#292)
    
    * Enabling the blocking provider and blocking consumer to be installed for device
    
    * Enabling the blocking provider and blocking consumer to be installed for device

[33mcommit 93ff3502678a7e2129c14af754972bb9424821a5[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Tue Jun 24 14:57:28 2025 -0400

    RDKEMW-5515 - Add Coverity Support for rbus (#291)
    
    Reason for change - Added coverity support for rbus
    ---------
    
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 5d344484211e31514610381f9089cd1cb4c65ff0[m[33m ([m[1;33mtag: [m[1;33mv2.5.0[m[33m)[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Fri Jun 20 12:37:46 2025 -0400

    Release of v2.5.0 (#263)
    
    * Release of v2.5.0
    ---------
    
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 5e4391ceaeccac992c30eea2662521d94146c1e0[m
Author: mdalal705 <167753445+mdalal705@users.noreply.github.com>
Date:   Thu Apr 25 18:27:29 2024 +0530

    RDKC-14860 : Remove the build scripts used in legacy Projects (#204)

[33mcommit 4e607497680efd15eebf69f044bacea9ac574250[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Wed Jun 18 23:01:09 2025 +0530

    RDKB-59911 Publish code-coverage as a part of workflow (#278)
    
    * RDKB-59911 Publish code-coverage as a part of workflow
    
    Reason for change: Added the codecoverage in workflow
    Test Procedure: Tested and verified
    Signed-off-by: RoseMary_Benny@comcast.com
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 3ba6c9ac97ace1fe36aae8aa88cafcf82b3441f0[m
Author: WeiShun Wang <tkqhlno@gmail.com>
Date:   Wed Jun 18 23:27:01 2025 +0800

    Fix rbusTableConsumer setting data failure (#268)
    
    * Fix rbusTableConsumer setting data failure
    
    The rbusTableConsumer sample was adding duplicate rows with the same aliases ("colors" and "shapes") to T1 table.
    
    This caused subsequent rbus_setStr() calls to fail when using path names composed with instance numbers like "instColors" or "instShapes".
    
    * Fix rbusTableConsumer setting data failure
    
    The rbusTableConsumer sample was adding duplicate rows with the same aliases ("colors" and "shapes") to T1 table.
    
    This caused subsequent rbus_setStr() calls to fail when using path names composed with instance numbers like "instColors" or "instShapes".

[33mcommit 9d63c46e3c0e394afb79ec0fb9fc2ca0347821d3[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Wed Jun 18 07:15:25 2025 +0530

    RDKB-59999 : Drop Incoming Request when Provider is NOT Responding (#283)
    
    * RDKB-59999 : Drop Incoming Request when Provider is NOT Responding
    
    Signed-off-by: dshett549 <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    
    * To fix jenkins error
    
    Signed-off-by: dshett549 <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    
    ---------
    
    Signed-off-by: dshett549 <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    
    RDKB-59999 : Drop Incoming Request when Provider is NOT Responding/Blocked (#269)
    
    Reason for change: Drop Incoming Request when Provider is NOT Responding/Blocked. When the request posted to the connection/callback_thread but if Provider is NOT consuming, the messages are queued up and eventually it leads to out-of-memory for the whole system. So It is decided to DROP all the incoming messages when the Queue size is 25.
    Test Procedure: Added Test Programs to simulate
    Priority: P0
    Risks: High
    Signed-off-by: dshett549 <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    Co-Authored-By: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 59a44bb7685af26f3817723a8da6aa328325bc71[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Tue Jun 17 17:47:14 2025 +0530

    RDKB-60109: Fix AddressSanitizer leaks (#275)
    
    * RDKB-60109: Fix AddressSanitizer leaks
    
    Reason for change: To avoid leaks
    Test Procedure: Build and verified
    Priority: P1
    Risks: Low
    
    Signed-off-by: dshett549 <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    
    * Fix Stray error /302 in code
    
    Signed-off-by: dshett549 <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    
    ---------
    
    Signed-off-by: dshett549 <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit c6ecc005b80ae1149ff57b2128967c4f8acf2ff9[m
Author: rosemarybennyy <RoseMary_Benny@comcast.com>
Date:   Wed Jun 4 15:50:27 2025 +0530

    RDKB-59819 Run Valgrind in the RBUS Workflow (#264)
    
    * RDKB-59819 Run Valgrind in the RBUS Workflow
    
    Reason for change: Added valgrind for github workflow
    Test Procedure: Tested and verified
    Signed-off-by: RoseMary_Benny@comcast.com
    
    * Update valgrind-test.yml
    
    * Update valgrind-test.yml
    
    ---------
    
    Signed-off-by: RoseMary_Benny@comcast.com
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 385ec982cc4034a1a7709fba2dd273f62cc9ec21[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Thu May 29 18:38:55 2025 +0530

    Fix memory leak in _setMultiInternal function (#265)
    
    Reason for change : memory leak in setmultiInternal when rbus_discoverComponentName returns invalid_input.

[33mcommit 94bc8ca3e1117a23961ca70226cc2d950d5c0517[m[33m ([m[1;33mtag: [m[1;33mv2.4.0[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Tue May 20 16:02:16 2025 -0400

    Release of v2.4.0
    
    Release of v2.4.0

[33mcommit 40017228ac95e29c2a136fd9b4b6eb1a79bf41de[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Wed May 21 01:24:37 2025 +0530

    Making rbusBuffer in public header (#262)
    
    Making rbusBuffer in public header

[33mcommit 7f640751b917734b942c4882e094e6d34fd5e3b5[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Fri May 16 21:49:02 2025 +0530

    Added test code for rbus_SetMulti rollback and rbus_getEXT functions (#260)

[33mcommit 172b46cc85f3dd01b5ede78435573d8a2bfff73b[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Tue May 13 20:25:37 2025 +0530

    RDKB-59722 : memory leak in rbus_getExt (#258)
    
    Reason for change: Identified a memory leak in rbus_getExt() when it called with single linear parameter.
    Test Procedure: run with valgrind
    Risks: low
    ---------
    
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 39b6ab3fb848ccd3bc85c5bbbf8d29a645376081[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Fri May 9 19:55:56 2025 +0530

    RDKB-59706 : Fix memory leak in _setMultiInternal (#259)
    
    Reason for change: Fix memory leak in _setMultiInternal when rollback is failing
    Test Procedure: Run valgrind
    Risks: Medium
    
    RDKB-59706 : Fix memory leak in _setMultiInternal (#257)
    
    Reason for change: Fix memory leak in _setMultiInternal when rollback is failing (#257)
    Test Procedure: Run valgrind
    Risks: Medium

[33mcommit e6c20809b076c8e74c91bf59424867f186f30cb3[m
Author: Vivek Singh Narwaria <132376676+viveksinghnarwaria@users.noreply.github.com>
Date:   Wed Apr 23 17:11:06 2025 -0400

    Reason for change: (#250)
    
    When a dmcli query is made with a large number of dots, rtrouted crashes. For example: dmcli eRT getv Device.NAT.X_CISCO_COM_DMZ.InternalIP
    
    The reason is that the tokenizeExpression function splits the datamodel path based on dots and writes into the workTokens array without checking the number of elements being written.
    This causes a buffer overflow if the number of dots is more than the size of the workTokens array (32).
    [Solution]
    Adding expression validation at the time of routeAdd to safe return with Invalid Expression Error.
    Test Procedure: Sanity.
    Risks: None.
    
    Signed-off-by: Vivek Narwaria Vivek_Narwaria@comcast.com
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit ed30640c74511681e8d2cc3566b46dfb28de8323[m[33m ([m[1;33mtag: [m[1;33mv2.3.0[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Fri Apr 11 09:35:22 2025 -0400

    Release of v2.3.0
    
    Release of v2.3.0

[33mcommit e1b952f1776cf82937f1babc7cb814e8ae2beb7b[m
Author: Lukasz Zwolinski (Cognizant) <lukasz.zwolinski@cognizant.com>
Date:   Fri Apr 11 14:59:06 2025 +0200

    RDKBDEV-2306: Insufficent RBus message header (#222)
    
    * RDKBDEV-2306: Insufficent RBus message header
    
    Reason for change:
    Sufficient size to be able to display Device.WiFi.DataElements paths
    
    * Addressed review comment

[33mcommit 4ef1b3217be8c6f8f95c5512e9da848cf08668f8[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Wed Apr 9 19:25:05 2025 +0530

    Modified boundary testing code for MAX_OBJECT_NAME_LENGTH (#254)
    
    Modify the unit test code to use Macro instead of hardcoded value

[33mcommit 7beed27ffe4bf225b445f250f06930157c0ffaa1[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Fri Apr 4 20:13:37 2025 +0530

    RDKB-59378 : Prevent destination discovery in rbus_getExt and rbus_setMulti for single parameter (#253)
    
    * Updated rbus_setMultiExt & rbus_getExt for single parameter

[33mcommit 8598d489414e7670d983690ccf1ec87be08d4a1d[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Wed Mar 26 15:44:50 2025 +0530

    RDKB-59246: Multiple components are crashing in "rbusValue_SetFromString" (#252)
    
    Reason for change: Multiple components are crashing after bootup where it is trying to dereference the NULL pointer.
    
    Signed-off-by: Deepthi C Shetty <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 2b4d509b8e8d2f46d78c909f0ee92f7d4993de50[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Thu Mar 6 07:17:41 2025 +0530

    RDKB-57891: Rollback support of DM parameters handled in RBUS (#251)
    
    Reason for change: while setting multiple values, if any DML values fails to set, others DMLs were being set with new value.
    
    Signed-off-by: Deepthi C Shetty <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 200e91229ceb476cdb0d3252b12d719eea34c0a3[m[33m ([m[1;33mtag: [m[1;33mv2.2.0[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Mon Mar 17 10:45:21 2025 -0400

    Release of v2.2.0
    
    Release of v2.2.0

[33mcommit f1846e67d7c4942cd15a9fbfa10084c730bd452c[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Thu Feb 13 08:36:18 2025 +0530

    Updated SetMulti API to use 'rbusHandle_FetchSetMultiTimeout' for timeout value (#249)
    
    * Updated SetMulti function with 60 sec timeout value
    
    * Update rbus_handle.c

[33mcommit b9a78fd4e2b5ef861ff0a700d66399dddd96d989[m
Author: pradeeptakdas <37864641+pradeeptakdas@users.noreply.github.com>
Date:   Wed Feb 12 10:52:03 2025 -0500

    RDKBDEV-3078: minor code cleanup of boolean parsing (#246)
    
    Reason for change:
    minor code cleanup of boolean parsing in rbusValue_SetFromString()
    Testing a string's prefix and length is equivalent to testing the
    entire string.
    Test Procedure: Sanity.
    Risks: None.
    Signed-off-by: Andre McCurdy <armccurdy@gmail.com>
    
    Co-authored-by: Andre McCurdy <armccurdy@gmail.com>

[33mcommit d21e68235a31ef96ddb3bf35c113568356d31bf3[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Tue Feb 11 20:41:53 2025 +0530

    RBUS-modify log messages & log level (#248)
    
    * RBUS-modify log messages & log level
    
    * Update log messages
    
    * Updated method invoke error log message

[33mcommit 10e4a32faccbabc6566ba32258a8f379f2094a40[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Thu Jan 23 00:19:39 2025 +0530

    RDKB-58390: RBUS- Configure timeout values for each rbus_handle (#244)
    
    * RBUS- Configure timeout values for each rbus_handle
    
    * Added Unit_tests for TimeoutValues

[33mcommit 61d9450d81c5985503f1d5525da86e6f59b54145[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Mon Oct 28 19:16:46 2024 +0530

    RDKB-57601: Fix compilation error in unittest code (#229)
    
    Reason for change: Fix compilation error in unittest code.
    Test Procedure: Test and verified.
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepthi C Shetty <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 76624ef904070b0b52290dddfaa61ae4505d288e[m
Author: Alina Laba <158189620+alinalabasagemcom@users.noreply.github.com>
Date:   Wed Oct 23 05:50:39 2024 +0300

    RDKBDEV-2687 tableSyncHandler callback to avoid undefined behavior while unregistering a row (#207)
    
    Co-authored-by: SGM-BonDeVoyage <ostap.truba-ext@sagemcom.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 7d278ecaa8506892517c819d9a40ea426830faaf[m[33m ([m[1;33mtag: [m[1;33mv2.1.0[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Mon Jan 6 21:42:06 2025 -0500

    Release of v2.1.0
    
    Release of v2.1.0

[33mcommit 87d4197f7c91b828c430cd8d1035d6ee4af0adea[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Mon Dec 16 21:49:20 2024 +0530

    RDKB-57964: Delay in getting the ipv6 after WanManager Restart (#243)
    
    Reason for change: The subscriberID sent while publishing the initial value is invalid and that leads the subscriber side of the middleware to ignore the received message itself and causes no event received at the consumer/subscriber
    Test Procedure: As per RDKB-57964
    Risks: Low
    
    Signed-off-by: Deepthi C Shetty <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 8dd9cf230373ed4a802004bdad3cadddd7cccf86[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Sat Dec 14 01:35:39 2024 +0530

    Fix C++ standard error during gtest compilation (#242)
    
    Signed-off-by: Deepthi C Shetty <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 28a6db70153acfe11f83602485176d12274546a8[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Wed Nov 20 19:43:55 2024 +0530

    Rbus fix memory leak (#237)
    
    * : Handled error conditions and fixed memory leaks.

[33mcommit 73c1f4e503d13f6ecb2b84b1a75e5875ca669963[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Wed Nov 20 08:24:54 2024 +0530

    add missing mutex unlock at early exit rtThreadPool (#236)
    
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit d3ada6bc3ad99794a7da52e9588c0fc00b05e424[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Tue Nov 19 20:13:39 2024 +0530

    Added proper LOCK & missing UNLOCK at early exit for rbus_element (#235)

[33mcommit 55849800f46fa96c7c112f54573952e4de2faf37[m
Author: pradeeptakdas <37864641+pradeeptakdas@users.noreply.github.com>
Date:   Thu Nov 14 10:57:08 2024 -0500

    RDKBDEV-2975: fix error message typo (#227)
    
    Reason for change:
    fix error message typo: rbuscore_message.c:204
    rbusMessage_GetBytes unexpected date type 5 Update date -> data Test
    Procecedure: Sanity.
    Risks: None.
    Signed-off-by: mkamat <mkamat@libertyglobal.com>

[33mcommit d83264d0198cf52a9c894910e2d6a42ccdd28ba3[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Mon Oct 28 20:10:00 2024 +0530

    RDKB-57423: Observed errors when ran valgrind for rbusTestConsumer -a (#228)
    
    Reason for change: Fixed errors when ran valgrind for rbusTestConsumer -a
    Test Procedure: run rbusTestConsumer with valgrind
    Risks: None.
    
    Signed-off-by: Deepthi C Shetty <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 376d23176dd61b8ae5317d1a4402a40f8bcada54[m
Author: pradeeptakdas <37864641+pradeeptakdas@users.noreply.github.com>
Date:   Wed Oct 23 06:06:01 2024 -0400

    RDKBDEV-2828: A DOS line ending is \r\n. A Unix line ending is \n. (#224)
    
    Reason for change:
    The sequence \n\r is not a valid line ending on either system.
    Test Procedure: Sanity.
    Risks: None.
    Signed-off-by: Andre McCurdy <armccurdy@gmail.com>
    
    Co-authored-by: Andre McCurdy <armccurdy@gmail.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 97e4254d5204c55ed1069a210f7e19a9c9a50fc7[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Tue Oct 15 21:19:39 2024 +0530

    Implementation of RBUS Publish API to support the setCommit functionality (#220)
    
    * Updated provider side for METHOD_COMMIT
    * Implemented setcommit in rbuscli
    
    ---------
    
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 70e3e28d697c87ad131f6b143407f5f9b12b8a9d[m
Author: cbucht200 <100870495+cbucht200@users.noreply.github.com>
Date:   Tue Oct 8 10:38:19 2024 -0400

    RBUS Events are not received if handle is closed and reopened. (#221)
    
    Event dispatch thread state is maintained in a static global variable. When rbus handle is shutdown, the state is never set to false, so any future handles opened will not receive events.

[33mcommit d6949673d1892ff7f8a702817a1dc0393899f8bb[m
Author: pradeeptakdas <37864641+pradeeptakdas@users.noreply.github.com>
Date:   Thu Oct 3 10:17:02 2024 -0400

    RDKBDEV-2829: fix GetParameterAttributes RPC (#215)
    
    * RDKBDEV-2829: fix GetParameterAttributes RPC
    
    Reason for change:
    fix GetParameterAttributes RPC on root object returns fault code 9002 ( OFW-4277 )
    
    Test Procedure: Sanity.
    Risks: None.
    Signed-off-by: anil shetty <adshetty@libertyglobal.com>
    ---------
    
    Co-authored-by: anil shetty <adshetty@libertyglobal.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 11f4744f2d8a027e8f9e37c541a1875359e69f41[m
Author: pradeeptakdas <37864641+pradeeptakdas@users.noreply.github.com>
Date:   Thu Oct 3 09:50:05 2024 -0400

    RDKBDEV-2830: fix memory leak in rbus_invokeRemoteMethod (#216)
    
    * RDKBDEV-2830: fix memory leak in rbus_invokeRemoteMethod
    
    Reason for change:
    1,024 bytes in 1 blocks are definitely lost in loss record 318 of 350
    malloc (vg_replace_malloc.c:309)
    rbus_getOpenTelemetryContextFromThreadLocal (rbuscore.c:2276)
    rbus_getOpenTelemetryContextFromThreadLocal (rbuscore.c:2269)
    rbus_getOpenTelemetryContext (rbuscore.c:2290)
    rbus_invokeRemoteMethod (rbuscore.c:1064)
    
    Test Procedure: Sanity.
    Risks: None.
    
    Signed-off-by: BijeeshaVS <bvattakaithayilsudha.contractor@libertyglobal.com>
    
    * Update rbuscore.c
    
    ---------
    
    Signed-off-by: BijeeshaVS <bvattakaithayilsudha.contractor@libertyglobal.com>
    Co-authored-by: Andre McCurdy <armccurdy@gmail.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 7aed660ebd3d4c8e87aa6d0efc0c7eecee56e0c5[m
Author: pradeeptakdas <37864641+pradeeptakdas@users.noreply.github.com>
Date:   Wed Oct 2 15:17:04 2024 -0400

    RDKBDEV-2827: check scanf() return values to avoid compiler warnings (#213)
    
    Reason for change:
    avoid compiler warnings
    Test Procedure: Sanity.
    Risks: None.
    
    Signed-off-by: Andre McCurdy <armccurdy@gmail.com>
    Co-authored-by: Andre McCurdy <armccurdy@gmail.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 18788061dfd625ef70837da2edb8e450e436b11e[m
Author: pradeeptakdas <37864641+pradeeptakdas@users.noreply.github.com>
Date:   Wed Oct 2 15:07:13 2024 -0400

    RDKBDEV-2826: check fread() return values to avoid compiler warnings (#212)
    
    Reason for change:
    void compiler warnings
    Test Procedure: Sanity.
    Risks: None.
    
    Signed-off-by: Andre McCurdy <armccurdy@gmail.com>
    Co-authored-by: Andre McCurdy <armccurdy@gmail.com>

[33mcommit 0eb1f563e02bf012f19a61ce459766618f75acaa[m
Author: garumu177 <139160677+garumu177@users.noreply.github.com>
Date:   Wed Sep 18 21:10:18 2024 +0530

    RDKC-15260 : Device not fetching MAC (#219)

[33mcommit 8fc2566a3112ea14fe74d3c44d5f7cf229f0103c[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Fri Jul 26 09:09:50 2024 +0530

    Handling null event Data (#210)
    
    Reason for change: : Handling null event data while appending to or
                                      updating from a message
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com

[33mcommit 1e9846687a075629d43d1e0e921b8d04e7a40db7[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Wed Jul 17 06:18:22 2024 +0530

    RDKB-56078:Move session manager to use rbus APIs (#211)
    
    * RDKB-56078:Move session manager to use rbus APIs
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com
    
    * Updated session manager gtest code
    
    * Updated unit test code

[33mcommit e07da2e16ff14f79efaf983ea4836ad36d3b8cec[m[33m ([m[1;33mtag: [m[1;33mv2.0.13[m[33m, [m[1;31morigin/rbus-2.0[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Thu Jul 11 08:54:21 2024 -0400

    Release of v2.0.13
    
    Release of v2.0.13

[33mcommit 22a5e4caff566cdcf26f194e45bc5f5f8c6164e3[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Fri Jun 28 08:16:15 2024 +0530

    "rbuscli get Device.WiFi." failed with Segmentation fault (#209)
    
    Reason for change: The crash occurred due to an invalid memory
        access. The rbusProperty_Release() function is to release local
        variables that have previously been freed.
        To resolve the problem, the local variable was initialized with "NULL".
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com

[33mcommit 17943e25d4177d4a84e84aa3f7e59f759e56148e[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Fri Jun 21 21:43:00 2024 +0530

    RDKB-55856:DM is not working with CPE interface alias name (#208)
    
    Reason for change: The getExt function returns a success error code if it is partially successful.
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com

[33mcommit 191e3f90e20763304fde8184434b529d0b196370[m
Author: pganna856 <prabhudas_gannavarapu@comcast.com>
Date:   Thu May 23 18:29:10 2024 +0530

    REFPLTB-3035: Memory leak in rbus "rbus_discoverRegisteredComponents" (#206)
    
    Reason for change: Fix Memory leak in rbus code.
        Test Procedure: Test and verified with rbuscli.
        Risks: Medium
        Priority: P1
    
        Signed-off-by: Prabhudas Gannavarapu <prabhudas_gannavarapu@comcast.com>

[33mcommit 6507ff329f25dec2c7ba55ba4cd01bbe41c7b36c[m[33m ([m[1;33mtag: [m[1;33mv2.0.12[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Mon Apr 15 15:35:32 2024 -0400

    Release of v2.0.12
    
    Release of v2.0.12

[33mcommit b510906bf68aef332cb195419ded8f87917fa76f[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Thu Apr 4 21:13:59 2024 +0530

    Fix: Modifying Error Codes in rbus_getExt API (#202)
    
    * Fix: Modifying Error Codes in RBUS Core
    
    Reason for change: When an object not found error originates from rtrouted,
                       rbuscore retrus error code RBUSCORE_ERROR_ENTRY_NOT_FOUND instead of RBUSCORE_ERROR_DESTINATION_UNREACHABLE.
    Signed-off-by: Netaji Panigrahi <Netaji_Panigrahi@comcast.com>
    
    * Fix: Modifying Error Codes in RBUS Core
    
    Reason for change: When an object not found error originates from rtrouted,
                       rbuscore retrus error code RBUSCORE_ERROR_ENTRY_NOT_FOUND instead of RBUSCORE_ERROR_DESTINATION_UNREACHABLE.
    Signed-off-by: Netaji Panigrahi <Netaji_Panigrahi@comcast.com>
    
    * Updated test code
    
    * Fixed Method_Invoke failed tests
    
    * Fixed SetMulti test
    
    * Fix: Modifying Error Codes in rbus_getExt API
    Reason for change: When Discover component names failed, rbusgetExt function will return the error code RBUSCORE_ERROR_ENTRY_NOT_FOUND.
    Signed-off-by: Netaji Panigrahi <Netaji_Panigrahi@comcast.com>
    
    * Fix: Modifying Error Codes

[33mcommit a80377bcbacde075c4e0954abee1f39bb8ee38ea[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Wed Apr 3 21:19:33 2024 +0530

    Fix: Modifying Error Codes in RBUS Core (#201)
    
    * Fix: Modifying Error Codes in RBUS Core
    
    Reason for change: When an object not found error originates from rtrouted,
                       rbuscore retrus error code RBUSCORE_ERROR_ENTRY_NOT_FOUND instead of RBUSCORE_ERROR_DESTINATION_UNREACHABLE.
    Signed-off-by: Netaji Panigrahi <Netaji_Panigrahi@comcast.com>

[33mcommit f82852235a0f3cd103edb05481396965ca5fdf78[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Tue Mar 26 23:14:20 2024 +0530

    RDKB-54362: Fix Memory Leak in gtest (#200)
    
    Reason for change: Fix Memory leak in rbus code.
    Test Procedure: Test and verified.
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepthi C Shetty <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 03c09d3bbf9e27898285b274e6b2b908551bb138[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Mon Mar 25 21:33:49 2024 +0530

    RDKB-54362: Fix Memory Leak in SampleApps, TestSuites and gtest (#199)
    
    Reason for change: Fix Memory leak in rbus code.
    Test Procedure: Test and verified with rbuscli and testapp.
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepthi C Shetty <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 69488129731261f3fb048f9d7559ef3dbed0fa7b[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Sat Mar 23 01:25:58 2024 +0530

    RDKB-54519: RBUS - rbusValue_ToString converts parameter boolean value into "true/false" strings (#198)
    
    * changed rbusValue_ToString to print true or false for Boolean
    
    * Updated value test code
    
    * Updated rbusValueTest.cpp

[33mcommit 1fdecc5c47a0f094c15e01d5c662ac48c88a2b7f[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Fri Mar 15 19:03:47 2024 +0530

    RDKB-54346: Add AddressSanitizer as a CMake build option for the rbus code (#197)
    
    Reason for change: Add AddressSanitizer as a CMake build option for the rbus code
    Test Procedure: Test and verified.
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepthi C Shetty <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 96b875a3f30ddd58ac590e7780bd278a5191dcb4[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Tue Mar 12 19:23:16 2024 +0530

    RDKB-54115: rbus_gtest.bin on RDKB device (#196)
    
    Reason for change: run rbus_gtest.bin on device
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepthi C Shetty <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 4bd91564fb534ce9c5aa3caffa6f8c714e5ed5b6[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Fri Feb 16 09:45:56 2024 +0530

    RDKB-53752: Add CodeCoverage in Github workflow (#195)
    
    * RDKB-53752: Add CodeCoverage in Github workflow
    
    Reason for change: Add CodeCoverage in Github workflow
    Test Procedure: Check Codecoverage in github
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepthi C Shetty <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    
    * Update unit_tests.yml
    
    ---------
    
    Signed-off-by: Deepthi C Shetty <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit f8e603f90b2f7d9c93b3251ecf07eef0f24875f0[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Thu Feb 15 19:44:32 2024 +0530

    RDKB-53593: Increased the get timeout to 120s for wildcard query (#193)
    
    * RDKB-53593: Use rbus API for get in CcspCommonLibrary
    
    Reason for change: Increased the get timeout to 120s for wildcard query
    Test Procedure: Check for CCSP_ERR_TIMEOUT while doing dmcli query.
    Risks: Low
    Priority: P1
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com

[33mcommit 290dec43dd27c63a9076a67abc7b37224729673f[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Wed Feb 7 19:57:09 2024 +0530

    Fix Unittest cases failure (#194)
    
    Signed-off-by: Deepthi C Shetty <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit e83e6a72bff66a5882b5c103bb8678aafce8cdec[m[33m ([m[1;33mtag: [m[1;33mv2.0.11[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Fri Jan 12 13:30:11 2024 -0500

    Release of v2.0.11
    
    Release of v2.0.11

[33mcommit dd0e7f547296157f3d43068903c3ccc936f091c9[m
Author: George Sanger <paperclipstudio@gmail.com>
Date:   Tue Jan 9 20:03:54 2024 +0000

    Fixed typo in rbuscli.c help messages (#185)

[33mcommit fa65f44c3e7e371b91b1c9864e52308efe74af9d[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Tue Nov 28 16:26:10 2023 -0500

    RDKB-51547 : RawData Direct mode with subscription Id (#186)
    
    * RDKB-51547 : RawData Direct mode with subscription Id
    
    Reason for change: Removed topic created with subscriptionId for
    normal subscription and subscriptionId based topic is only created for
    rawdata direct mode subscription
    Test Procedure: Test and verified with rbuscli and testapp
    Risks: High
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>
    
    * RDKB-51547 : RawData Direct mode with subscription Id
    
    Reason for change: Removed topic created with subscriptionId for
    normal subscription and subscriptionId based topic is only created for
    rawdata direct mode subscription
    Test Procedure: Test and verified with rbuscli and testapp
    Risks: High
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>
    
    RDKB-51547 - [On rbusDirect] Implementation of RBUS Publish API to send raw data (#191)
    
    * RDKB-51547 - [On rbusDirect] Implementation of RBUS Publish API to send Raw Data
    
    Reason for change: Added sample apps for rawdata subscriptions
    Test Procedure: Build and verified
    Risks: Low
    Priority: P1
    Signed-off-by: Gururaaja ESR <Gururaja_ErodeSriranganRamlingham@comcast.com>
    
    * RDKB-51547 - [On rbusDirect] Implementation of RBUS Publish API to send Raw Data
    
    Reason for change: Added sample apps for rawdata subscriptions
    Test Procedure: Build and verified
    Risks: Low
    Priority: P1
    Signed-off-by: Gururaaja ESR <Gururaja_ErodeSriranganRamlingham@comcast.com>
    
    RDKB-51547 - [On rbusDirect] Implementation of RBUS Publish API to send Raw Data (#190)
    
    Reason for change: Fixed the ssh failure issue reported in CI testing.
    Test Procedure: Build and verified
    Risks: Low
    Priority: P1
    Signed-off-by: Gururaaja ESR <Gururaja_ErodeSriranganRamlingham@comcast.com>
    
    RDKB-51547 : [On rbusDirect] Implementation of RBUS Publish API to send Raw Data (#187)
    
    Implemented rtConnection_AddListener() and rtConnection_AddListenerWithUID()
    
    ---------
    
    RDKB-51547 : RawData Direct mode with subscription Id (#176)
    
    * RDKB-51547 : RawData Direct mode with subscription Id
    
    Reason for change: Implemented rawdata subscription on direct mode
    with unique subscription Id for each subscription
    Test Procedure: Test and verified with rbuscli and testapp
    Risks: High
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>
    
    * RDKB-51547 : RawData Direct mode with subscription Id
    
    Reason for change: This commit inculded below changes
    1.  Raw data subscription is only for event type
    2.  Added unique subscription Id for each subscription in normal and direct mode.
    3.  Removed support of add listener, remove listener and send message from rbuscli.
    4.  Addlistener topic name includes subscriptionId.
    5.  Allowed adding multiple listener of same topic.
    6.  Removing listener based on the subscription Id.
    7.  Changes are made to add topic name appropriately when we switch between normal mode and direct mode.
    8.  Rawdata and subscriptionId data are maintained in the internal event subscription structure maintained by consumer.
    9.  The subscriptionId from the provider is send to the consumer through the callback handler.
    10. Removed master_event_callback(), as individual subscription will invoke the new callback handler(_consumer_event_handler).
    Test Procedure: Test and verified with rbuscli and testapp
    Risks: High
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>
    
    * rbus_publishSubscriberEvent
    
    Updated the lock
    
    ---------
    
    Co-Authored-By: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
    Co-Authored-By: gururaajar <83449026+gururaajar@users.noreply.github.com>

[33mcommit bb16d74c082dd4598c9fe10d2cf4dc8a6796e790[m[33m ([m[1;33mtag: [m[1;33mv2.0.10[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Tue Dec 26 11:49:59 2023 -0500

    Release of rbus v2.0.10
    
    Release of rbus v2.0.10

[33mcommit 6b17b8ef390ec7b42594e1535094c71cea1aeaac[m
Author: sheabot <sheabot03@gmail.com>
Date:   Thu Nov 9 13:50:08 2023 -0700

    fix: #include rbus headers using quotes instead of angle brackets to search same directory instead of system include paths (#169)

[33mcommit bd340c318a222668a202fbfb069ac566741c4f2d[m
Author: Deepak M <114924314+dm097@users.noreply.github.com>
Date:   Wed Nov 8 22:20:37 2023 +0530

    RDKB-51761 : Test Suite Enhancements (#181)
    
    * RDKB-51761 Test scenario to cover multiple rbus_open() call
    
    Reason for change: Need to cover test app and unit test to have a test cases which covers multiple rbus_open() with same component name without rbus_close() call.
    The test scenario should also cover regDataElements, rbus_set, rbus_get, addrow and methodinvoke after doing multiple rbus_open() function call
    Test Procedure: Start rtrouted and ./rbus_test.sh OR verify with unit_test.yml
    Risks: Medium
    Priority: P1
    Signed-off-by: Deepak_m <deepak_m@comcast.com>
    
    RDKB-51761 : Test Suite Enhancements  (#175)
    
    * RDKB-51761 Test scenario to cover multiple rbus_open() call
    
    Reason for change: Need to cover test app and unit test to have a test cases which covers multiple rbus_open() with same component name without rbus_close() call.
    The test scenario should also cover regDataElements, rbus_set, rbus_get, addrow and methodinvoke after doing multiple rbus_open() function call
    Test Procedure: Start rtrouted and ./rbus_test.sh
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepak_m <deepak_m@comcast.com>
    
    * RDKB-51761 Test scenario to cover multiple rbus_open() call
    
    Reason for change: Need to cover test app and unit test to have a test cases which covers multiple rbus_open() with same component name without rbus_close() call.
    The test scenario should also cover regDataElements, rbus_set, rbus_get, addrow and methodinvoke after doing multiple rbus_open() function call
    Test Procedure: Start rtrouted and ./rbus_test.sh
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepak_m <deepak_m@comcast.com>
    
    * RDKB-51761 Test scenario to cover multiple rbus_open() call
    
    Reason for change: Need to cover test app and unit test to have a test cases which covers multiple rbus_open() with same component name without rbus_close() call.
    The test scenario should also cover regDataElements, rbus_set, rbus_get, addrow and methodinvoke after doing multiple rbus_open() function call
    Test Procedure: Start rtrouted and ./rbus_test.sh
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepak_m <deepak_m@comcast.com>
    
    * RDKB-51761 Test scenario to cover multiple rbus_open() call
    
    Reason for change: Need to cover test app and unit test to have a test cases which covers multiple rbus_open() with same component name without rbus_close() call.
    The test scenario should also cover regDataElements, rbus_set, rbus_get, addrow and methodinvoke after doing multiple rbus_open() function call
    Test Procedure: Start rtrouted and ./rbus_test.sh
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepak_m <deepak_m@comcast.com>
    
    * Update unit_tests.yml
    
    RDKB-51761 Test scenario to cover multiple rbus_open() call
    
    Reason for change: added unit tests verified on GitHub yml
    Test Procedure: very unit_tests.yml success
    Risks: Medium
    Priority: P1
    
    ---------
    
    Signed-off-by: Deepak_m <deepak_m@comcast.com>
    
    RDKB-51761 : Test Suite Enhancements (#179)
    
    * RDKB-51761 Test scenario to cover multiple rbus_open() call
    
    Reason for change: Need to cover test app and unit test to have a test cases which covers multiple rbus_open() with same component name without rbus_close() call.
    The test scenario should also cover regDataElements, rbus_set, rbus_get, addrow and methodinvoke after doing multiple rbus_open() function call
    Test Procedure: Start rtrouted and ./rbus_test.sh OR verify with unit_test.yml
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepak_m <deepak_m@comcast.com>
    
    RDKB-51761 : Test Suite Enhancements (#184)
    
    * RDKB-51761 Test scenario to cover multiple rbus_open() call
    
    Reason for change: Need to cover test app and unit test to have a test cases which covers multiple rbus_open() with same component name without rbus_close() call.
    The test scenario should also cover regDataElements, rbus_set, rbus_get, addrow and methodinvoke after doing multiple rbus_open() function call
    Test Procedure: Start rtrouted and ./rbus_test.sh OR verify with unit_test.yml
    Risks: Medium
    Priority: P1
    
    RDKB-52662 Update the multi rbusopen Tests (#188)
    
    Reason for change: Fix for multiRbusOpenRbusGetProvider crash on device
    Test Procedure: Start rtrouted and ./rbus_test.sh OR verify with unit_test.yml
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepak_m <deepak_m@comcast.com>

[33mcommit 7e4f0be6ba6b190dbacd2d2f237de17e70c4f28b[m
Author: Deepak M <114924314+dm097@users.noreply.github.com>
Date:   Mon Nov 20 02:54:58 2023 +0530

    LTE-2047 [AUTO][Release][6.7]Observed cellularmanager crash with signature 85788042 and reason as SIGSEGV /SEGV_MAPERR after 3 days of uptime (#183)
    
    Reason for change: Observed cellularmanager crash with signature 85788042 and reason as SIGSEGV /SEGV_MAPERR after 3 days of uptime
    Test Procedure: Testing mentioned in Jira description
                    1)XLE is with flashed with 6.7p1s6 release build
                    2)XB is flashed with 6.2p26s1 release build
                    3)XLE is an extender connected to XB via wifiBH
                    4)2 clients connected to the setup
    Risks: Low
    Priority: P1
    
    Signed-off-by: Deepak_m <deepak_m@comcast.com>

[33mcommit 11ffaef598edfe87790eabc7f4c7bb13bf4e6c29[m[33m ([m[1;33mtag: [m[1;33mv2.0.9[m[33m)[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Fri Nov 10 20:22:35 2023 +0530

    Update rtrouted.c (#180)
    
    (cherry picked from commit 4e42012c893788282cc3193dc6faa8f6e9d24a1b)

[33mcommit 851a6f61897d382ae5f0dba5f2b2043c6a2e86b1[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Thu Nov 9 15:26:04 2023 -0500

    Release of rbus v2.0.9
    
    Release of rbus v2.0.9

[33mcommit 62cd231b5bc18134ba64551deb70cb808a87f7f9[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Thu Nov 9 01:46:31 2023 +0530

    LTE-2173:[XLE][23Q4] XLE and XB are in split brain mode always (#178)
    
    * LTE-2173: [XLE][23Q4] XLE and XB are in split brain mode always
    
    Reason for change: Because of a permissions issue with the /tmp/.rbus directory, the provider is unable to access the subscribers' component ID file.
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com
    
    * Updated debug message to error
    
    * LTE-2173:[XLE][23Q4] XLE and XB are in split brain mode always
    
    Reason for change: Because of a permissions issue with the /tmp/.rbus directory, the provider is unable to access the subscribers' component ID file.
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com

[33mcommit 885f1f629ef80991e29b9e0046d4b47c833b30ad[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Wed Nov 1 00:37:26 2023 +0530

    RBUS-Update logging messages (#174)
    
    * RBUS-Update logging messages
    
    Reason for change: Removing unwanted function names from logging
    messages
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com

[33mcommit dcd0ef3ac6361ff64342da2bcf48669afe859c29[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Sat Oct 21 21:30:14 2023 +0530

    RDKB-50101:RBUS Event Producer can have multiple instances of the same Subscriber (#172)
    
    Reason for change: While recovering from a crash, the provider is unaware of a valid rbus_open connection of consumer,so it lead to duplicate entres of subscriptions. Before loading old subscriptions from the cache file, a "/tmp/.rbus/<ConsumerPID>_<ComponentId>" file existence check is now performed.
    
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com
    
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com

[33mcommit 97679ac541e636af37bb6759a82d4158350bca6d[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Wed Oct 18 21:57:36 2023 +0530

    RDKB-51891:Observed "OneWifi" crash with with Fingerprint:61377904 (#171)
    
    Reason for change: In rbusEvent_SubscribeWithRetries, using subInternal is not mutex protected.As a result,
    when unsubscription occur at the same time and it removed subInternal from the List leads to crash.
    Using subInternal under mutex protection to avoid crash.
    
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com

[33mcommit e35eebcc9613b7db477c83da823384f2ea418b7a[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Tue Oct 17 19:27:20 2023 +0530

    RDKB-52005: RBUS - Use separate mutex for Provider & Consumer side subscriptions list. (#170)
    
    Reason for change: Used common mutex for both Provider & Consumer side subscriptions lists,
    so added separate mutex variable for both Provider & Consumer.
    
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com

[33mcommit f62bdeead7e90db8fa00c64479703dc87d2ad96e[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Wed Oct 11 21:03:22 2023 +0530

    CMXB7-5636:Onewifi crash observed with function rbusAsyncSubscribeRetrier_CompareSubscription (#167)
    
    * CMXB7-5636:Onewifi crash observed with function rbusAsyncSubscribeRetrier_CompareSubscription
    
    Reason for change: AsyncSubscriptions list maintained in the consumer is not mutex protected while remove from the List.
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com

[33mcommit 56f12d2bc219f9fc0621349a0ba806801914e118[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Thu Oct 5 07:32:10 2023 +0530

    Observed provider side erro while using 'method_noargs' rbuscli command (#166)
    
    Reason for change: Reading 'inParms' in the _method_callback_handler function
    without adding 'inParms' in the request message in rbusMethod_InvokeInternal() results in an error.
    
    The 'hasInParam' variable was added to find out whether a request
    message has inParams or not.
    
    Test Procedure: Test and verified
    Risks: Low
    Priority: P1
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com

[33mcommit 1b176ed073e7a689fc276861064ca159f27032c9[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Thu Oct 5 01:25:45 2023 +0530

    RDKB-51370: Use the Disconnect Event Handler for rbus direct connections (#165)
    
    Reason for change: The Disconnect Event Handler registration has been utilised in rbus_openDirect and removed from other places
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com

[33mcommit a3895830e705e0e0460018c16eb848edbe4e7540[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Fri Sep 29 01:33:13 2023 +0530

    RDKB-51301 : RBUS - Add mutex-protected to the Consumer side's subscriptions list (#157)
    
    * RDKB-51301 : RBUS - Add mutex-protected to the Consumer side's subscriptions list
    
    Reason for change: The subscriptions list on the consumer side is not mutex protected.
                       As a result, when some component subscribes/unsubscribes the event,
                       it affects the subscription list at the same time.
    
    Test Procedure: Verified with TestProvider & TestConsumer
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com
    
    * Implement private function to handle Unsubscription events
    
    * Added dirty unsubscription for RawData Events
    Added rbusRawDataProvider sampleapp to test Rawdata subscription & publish
    
    * Unsubscription error code has been modified.
    
    * updated subscription error code

[33mcommit 594ef1f3756e6ba3b99c762b20e2979144a518e7[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Tue Oct 3 16:50:11 2023 -0400

    RDKB-51760 - Provider crash with multiple rbus_open() call (#164)
    
    Reason for change: With multiple rbus_open() call with same component name,
    we are closing and opening the handle again. After closing the handle,
    we are not performing rbusConfig_CreateOnce(), because of which in rbus_regDataElements()
    where we are calling rbusConfig_Get()->tmpDir leads to a NULL pointer deferencing.
    Made changes to call _rbus_open_pre_initialize() in the issue scenario.
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>

[33mcommit fabe55fc82cf82900c80bc0741cc66cc138faccf[m[33m ([m[1;33mtag: [m[1;33mv2.0.8[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Thu Sep 28 16:09:07 2023 -0400

    Release of v2.0.8
    
    Release of v2.0.8

[33mcommit 7cd9d4cf2ef58c851adf13235ac3886262776f89[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Tue Sep 26 06:37:54 2023 +0530

    Consumer side use dirty subscription record for second time subscription (#160)
    
    * Consumer side use dirty subscription record for second time subscription
    
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com
    
    * Update rbus.c
    
    ---------
    
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com

[33mcommit 4d1e7ec0aec4f190f494ac843235256a4cf9b4cf[m
Author: george-sanger <135598855+george-sanger@users.noreply.github.com>
Date:   Tue Sep 26 02:22:24 2023 +0100

    Added const to rbusValue_MarshallTMtoRBUS (#137)

[33mcommit 5bce4659e6dffae4456c4f45c4766d5cc07a3589[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Fri Sep 22 19:17:47 2023 +0530

    RDKB-51511: Turn Off AutoPublish feature for ValueChange (#159)
    
    * RDKB-51511: Turn Off AutoPublish feature for ValueChange
    
    Reason for change: By default AutoPublish is set to false for ValueChange
    Test Procedure: Test and verified
    Risks: Low
    Priority: P1
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com
    
    * Fixed failed sampleapps and rbusTestProvider
    
    Turn On AutoPublish feature by default (#162)
    
    Turn On AutoPublish feature by default

[33mcommit a4c23feba4d55007a88359bf4df70fb2b8ea40d1[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Tue Sep 19 10:13:38 2023 -0400

    RDKB-51386 : Addition of RBUS heart beat monitor service (#158)
    
    * RDKB-51386 : Addition of RBUS heart beat monitor service
    
    Reason for change: Added rbus_heartbeat.service file to monitor the status
    of rtrouted. Using rtrouted_diag, a separate parameter has been added for
    heartbeat monitoring which will just call rtConnection_sendmessage to confirm
    the rtrouted is running properly or not. If the sendmessage is failed,
    rtrouted will be killed and the device will be rebooted.
    Test Procedure: Tested by simulation
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>
    
    * RDKB-51386 : Addition of RBUS heart beat monitor service
    
    Reason for change: Added rbus_heartbeat.service file to monitor the status
    of rtrouted. Using rtrouted_diag, a separate parameter has been added for
    heartbeat monitoring which will just call rtConnection_sendmessage to confirm
    the rtrouted is running properly or not. If the sendmessage is failed,
    rtrouted will be killed and the device will be rebooted.
    Test Procedure: Tested by simulation
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>

[33mcommit 8369ca113e00170da12e8d3d5ad0de37a6eba856[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Thu Sep 7 19:04:06 2023 +0530

    RDKB-50101 : RBUS Event Producer can have multiple instances of the same Subscriber (#155)
    
    * RDKB-50101 : RBUS Event Producer can have multiple instances of the same Subscriber
    
    Reason for change: Handled UnSubscription of DML when provider is not running.
         If an event's unsubscription fails because a provider cannot be discovered, the subscription node is marked as dirty, and it will be resolved when the provider is active and publishes the event.
    
    Test Procedure: In terminal 1: Start rtrouted,
                    In terminal 2: run the ./rbusEventProvider.
                    In terminal 3: run rbuscli -i
             Test steps:
             1. Subscribe DML "Device.Provider1.Param1"
                 sub Device.Provider1.Param1
             2. In terminal 2 : Kill ./rbusEventProvider
             3. In terminal 3: unsub Device.Provider1.Param1
             4. In terminal 2, run the ./rbusEventProvider.
             5. In terminal 3: Verify "Device.Provider1.Param1" unsubscription will be success
    
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 0f3f481c953583c85539f4b9e41d80e7e17b57af[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Wed Sep 6 22:55:13 2023 +0530

    RDKB-51177:RBUS- API to find event subscription (#154)
    
    * RDKB-51177:RBUS-Implement a new method to support consumer to find whether they have active subscription already or not for the given DML with subscription structure.
    Test Procedure: verified with ./rbusTestProvider & rbusTestConsumer
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com
    
    * Changed rbusEvent_FindSubscription method name to rbusEvent_IsSubscriptionExist

[33mcommit 1d62eb55c1871b750e0f0e461d324b6a26f2c05e[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Thu Aug 31 12:10:39 2023 -0400

    RDKB-51121 : Onewifi crash observed with function strlen, rtLogPrintf (#152)
    
    Reason for change: For fixing the testapp build error while building with jenkins
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>
    
    RDKB-51121 : Onewifi crash observed with function strlen, rtLogPrintf (#151)
    
    Reason for change: Subscriptions list maintained in the provider side is not mutex protected. Because of which when some component subscribe/unsubscribe the event, which in turn affects the subscription list and at the same time.
    
    if provider tries to access the subscriptions list in rbusEvent_Publish
    it leads to a crash. Mutex protection added for subscription list to avoid the SIGSEGV.
    
    Test Procedure: Added Test Program to run subscribe and unsubscribe in while(1) from multiple threads. No reproduction of the issue means, its addressed.
    
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>

[33mcommit 56836d53f533fff991c7e067a7ac88eedf7970d6[m
Author: Deepak M <114924314+dm097@users.noreply.github.com>
Date:   Mon Aug 21 17:44:43 2023 +0530

    RDKB-50778 [AUTO][Stable2/6.3]Observing "rbusSubscriptions_saveCache" logs flooding in telemetry2_0.txt.0 (#149)
    
    * RDKB-50778 [AUTO][Stable2/6.3]Observing "rbusSubscriptions_saveCache" logs flooding in telemetry2_0.txt.0
    
    modifying a loglevel to DEBUG
    
    Signed-off-by: Deepak_m <deepak_m@comcast.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 892e4d562a7a3117e70e0b99f9bbb2c57daa54b2[m[33m ([m[1;33mtag: [m[1;33mv2.0.7[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Mon Jul 31 16:46:33 2023 -0400

    Release of RBUS v2.0.7
    
    Release of RBUS v2.0.7

[33mcommit 3c5d46adf4fbe878adaf8fd814ee09089c44fb7f[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Wed Jul 5 22:54:32 2023 -0400

    RDKB-48941 : Session Id support in ccsp (#132)
    
    * RDKB-48941 : Session Id support in ccsp
    
    Reason for change: Added session ID support in rbuscli along with the
    required changes in the session manager to publish the current session
    Id signal, which is subscribed in the ccsp common library
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>
    
    * RDKB-48941 : Session Id support in ccsp
    
    Reason for change: Added session ID support in rbuscli along with the
    required changes in the session manager to publish the current session
    Id signal, which is subscribed in the ccsp common library
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>
    
    ---------
    
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit a2a9fd7d851daa85d911ea1ea45e27255b69d13c[m
Author: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
Date:   Sat Jul 29 22:18:56 2023 -0400

    RDKCOM-4106 : Added support to dynamically add listener path (#146)
    
    Signed-off-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit 907eb93c9f630bcb67b20d937913a9d05db06fa5[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Tue Jul 25 21:25:25 2023 +0530

    RDKB-50426: Fix unittest failure in rbusTestConsumer (#144)
    
    Reason for change: unittest failure in rbusTestConsumer
    Test Procedure: Tested and verified
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 3c08ef7fbab8816809b93b0a2b69495900b8c2b5[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Tue Jul 18 19:48:30 2023 +0530

    RDKB-50193: rbusEvent_Publish is returning success even when the consumer is unsubscribed (#141)
    
    Reason for change: rbusEvent_Publish is returning success even when the consumer is unsubscribed
    Test Procedure: Tested and verified
    Risks: Medium
    Priority: P1

[33mcommit a10e584e5820a6e762519d3a6f078651c155cc47[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Mon Jul 10 18:34:15 2023 +0530

    RDKB-50100: Encoding, an rtMessage structure types are incorrect (#140)
    
    Reason for change: values are not encoded as uint values, but are being encoded as signed integers.
    Test Procedure: Tested and verified
    Risks: Medium
    Priority: P1
    
    Change-Id: Ifeae58ba6d421863056d15fdcadab74b659b1244
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 495a950360f2499d4844738aad896b5affd0c7f5[m
Author: Deepak M <114924314+dm097@users.noreply.github.com>
Date:   Mon Jul 3 19:35:21 2023 +0530

    RDKB-49347 Rbus should not allow Table Subscription that does not end with DOT. (#135)
    
    * RDKB-49347 Rbus should not allow Table Subscription if event name does not end with '.'(DOT)
    
    Reason for change: Subscription for event name of Table type should not be allowed if event name does not end with "."
    Test Procedure: 1)Rbus Rtrouted and rbusTestProvider  Or rbustableProvider; rbus rbuscli -i; sub Device.Tables1.T1 should fail and sub Device.Tables1.T1. should pass
    2) Rbus Rtrouted and rbusTestProvider and rbusTestConsumer, all test cases should pass
    
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepak_m <deepak_m@comcast.com>
    
    * RDKB-49347 Rbus should not allow Table Subscription if event name does not end with '.'(DOT)
    
    Reason for change: Subscription for event name of Table type should not be allowed if event name does not end with "."
    Test Procedure: 1)Rbus Rtrouted and rbusTestProvider  Or rbustableProvider; rbus rbuscli -i; sub Device.Tables1.T1 should fail and sub Device.Tables1.T1. should pass
    2) Rbus Rtrouted and rbusTestProvider and rbusTestConsumer, all test cases should pass
    
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepak_m <deepak_m@comcast.com>
    
    * Update rbus.c
    
    * Update rbus.c
    
    Updated Error Code handling
    
    * RDKB-49347 Rbus should not allow Table Subscription if event name does not end with '.'(DOT)
    
    Reason for change: Subscription for event name of Table type should not be allowed if event name does not end with "."
    Test Procedure: 1)Rbus Rtrouted and rbusTestProvider  Or rbustableProvider; rbus rbuscli -i; sub Device.Tables1.T1 should fail and sub Device.Tables1.T1. should pass
    2) Rbus Rtrouted and rbusTestProvider and rbusTestConsumer, all test cases should pass
    
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepak_m <deepak_m@comcast.com>
    
    * RDKB-49347 Rbus should not allow Table Subscription if event name does not end with '.'(DOT)
    
    Reason for change: Subscription for event name of Table type should not be allowed if event name does not end with "."
    Test Procedure: 1)Rbus Rtrouted and rbusTestProvider  Or rbustableProvider; rbus rbuscli -i; sub Device.Tables1.T1 should fail and sub Device.Tables1.T1. should pass
    2) Rbus Rtrouted and rbusTestProvider and rbusTestConsumer, all test cases should pass
    
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepak_m <deepak_m@comcast.com>
    
    ---------
    
    Signed-off-by: Deepak_m <deepak_m@comcast.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit e40ab67dcf31496cf380bf18cd8c99a67d5a1504[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Mon Jul 24 18:38:23 2023 +0530

    The output for the rbuscli -g option no longer contains the Carriage Return (\r). (#145)
    
    Add support for rbuscli with -g and -s options (#143)
    
    * Adding support for rbuscli with -g and -s options
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com
    
    * Update rbuscli.c
    
    Remove wildcard support for rbuscli -g option (#147)
    
    wildcard queries should not allow for rbuscli -g option

[33mcommit 3a90bda26077ca00629dc4d6b565d3e660008153[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Mon Jun 26 21:53:08 2023 +0530

    RDKC-13696: RBUS: Avoid using script/system commands using system functions (#134)
    
    Reason for change: Avoid using script/system commands using system functions
    Test Procedure:    Tested and verified
    Risks:             Medium
    Priority:          P1
    
    Change-Id: I6659ae803d998d692283b5504b9f37d4fe3854af
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit ce455ba53851f766f33b90513e682a2c6f52079a[m
Author: Deepak M <114924314+dm097@users.noreply.github.com>
Date:   Wed May 3 21:27:59 2023 +0530

    RDKB-48890 Addressing Unit test work flow on Github Rdkcentral Rbus (#126)
    
    Reason for change: Github unittest workflow is not capturing failure count of rbusTestConsumer
    Test Procedure: Trigger and verify Unittest work flow on rdkcentral rbus github
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepak <deepak_m@comcast.com>

[33mcommit 212b4eb62426ed2608aadfe8059ba32b049f9650[m[33m ([m[1;33mtag: [m[1;33mv2.0.6.1[m[33m)[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Wed Jul 12 16:42:40 2023 -0400

    Event nocopy to avoid foot prints of memory and CPU (#138)
    
    Implementation of APIs that simply copies raw data and does not use Serialization and Encoding.
    This is needed for Publisher who does not want to encode.
    
    ```
    rbusEvent_SubscribeRawData()
    rbusEvent_UnsubscribeRawData()
    
    rbusEvent_SubscribeExRawData()
    rbusEvent_UnsubscribeExRawData()
    
    rbusEvent_PublishRawData()
    ```
    
    ---------
    
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 787225f375ddbf79e4ec2d52f5132aeedd403591[m[33m ([m[1;33mtag: [m[1;33mv2.0.6[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Tue Jul 4 13:46:05 2023 -0400

    Release of rbus-2.0.6
    
    Release of rbus-2.0.5

[33mcommit 6ec8d0e17a6320f6b8bc533c05afc7f4da43e846[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Tue Jun 6 19:21:54 2023 +0530

    Registration of property as sibling to a table not allowed (#130)
    
    * Registration of property as sibling to a table not allowed and vice versa
    
    * Update rbus.c

[33mcommit 653c9200429f2beaee10c0cef2aca19ffc1d717f[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Fri Jun 16 20:16:32 2023 +0530

    openTelemetry sample app to take input from user  (#131)
    
    * openTelemetry sample app to take input from user and set that as traceparent&tracestate
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com
    
    * Update rbusOpenTelemetry.c
    
    Fixed typo
    
    * Added a new run_client function in openTelemetry sample app to take input from user
    
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com
    
    * Update rbusOpenTelemetry.c
    
    * Update rbusOpenTelemetry.c
    
    ---------
    
    Signed-off-by: Netaji Panigrahi Netaji_Panigrahi@comcast.com
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 139564acc0f2170c3720c6c0050f81156b7330e1[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Thu Jun 1 19:33:47 2023 +0530

    Additional 0 added for traceParent value: 00-foo-bar-0200 (#129)
    
    RDKB-48821: WEBCONFIG having additional 0 for traceParent value: 00-foo-bar-0200
    
    In rbus_setOpenTelemetryContext function ot_ctx->otTraceParent, ot_ctx->otTraceState variables set to '\0' before using.
    
    Signed-off-by: Netaji Panigrahi <Netaji_Panigrahi@comcast.com>

[33mcommit c2b1553160d468cca3de4f9b415b08cc53c892e2[m
Author: Deepak M <114924314+dm097@users.noreply.github.com>
Date:   Tue May 23 19:12:03 2023 +0530

    RDKB-48936 [23Q2_Sprint] Events Test Cases are Failing in rbusTestConsumer (#128)
    
    Reason for change: Reverting RDKB-46677 as 32 test cases of rbusTestConsumer was failing on Github unittest
    Test Procedure: Verify rbusTestConsumer on VM and Unittest work flow on rdkcentral rbus github
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepak_m <deepak_m@comcast.com>

[33mcommit 2ec44ae232d0bcdd0336c620f9da9b846874c87d[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Mon Jun 26 21:53:08 2023 +0530

    RDKC-13696: RBUS: Avoid using script/system commands using system functions (#134)
    
    Reason for change: Avoid using script/system commands using system functions
    Test Procedure:    Tested and verified
    Risks:             Medium
    Priority:          P1
    
    Change-Id: I6659ae803d998d692283b5504b9f37d4fe3854af
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 9c049bf778d2beb411c107d0d05781420c2466fa[m[33m ([m[1;33mtag: [m[1;33mv2.0.5[m[33m, [m[1;33mtag: [m[1;33mAML_BSP_REL_VERSION_RDK5.1.0[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Fri May 5 10:31:13 2023 -0400

    Release of rbus-2.0.5
    
    Release of rbus-2.0.5

[33mcommit 6714f03920558b16e10b582648f6261037c6bbd3[m
Author: Deepak M <114924314+dm097@users.noreply.github.com>
Date:   Mon May 1 19:18:47 2023 +0530

    RDKB-48378 : Must keep one active connection to Daemon until the last handle is closed (#124)
    
    * RDKB-48378 : Must keep one active connection to Daemon until the last handle is closed
    
    Reason for change: When a rbus_open component with same name is closed, rbus_closeBrokerConnection should not destroy any exiting active component session
    Test Procedure: 1)Run rtrouted;Run rbusSampleProvider and verify get and set of its components via rbuscli;
                    2)Run rbus_gtest and verify all Test passed in previous commit are passing with change
    Risks: Medium
    Priority: P1
    Signed-off-by: Deepak <deepak_m@comcast.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 3fd2625ed66f55641bb6e303784d2ca7335bf09a[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Mon Apr 24 19:45:01 2023 +0530

    Support interval-based & duration subscription for wildcard event (#120)
    
    * Support interval-based & duration subscription for wildcard event
    
    Test Procedure: Setup 3 terminals
        1. In terminal 1, Start rtrouted
            " killall -9 rtrouted; rm -fr /tmp/rtroute*; rtrouted -f -l DEBUG "
        2. In terminal 2, Run a sample app.
            " ./rbusTableProvider 200 "
        3. In terminal 3, run " ./rbucli -i "  with following commands
            Add rows to the table
                " addr Device.Tables1.T1. "
                " addr Device.Tables1.T1. "
            Subscribe wildcard event with interval & duration
                " subint Device.Tables1.T1.*.Data 5 10 "
    
    Signed-off-by: Netaji Panigrahi <Netaji_Panigrahi@comcast.com>

[33mcommit 1cc6084ae029ee5c5128d7f773d10993ed35dde1[m
Author: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
Date:   Mon Apr 10 16:37:13 2023 -0400

    Implementation of Private Connection Support (#117)
    
    * Implementation of Private Connection Support
    * Added SampleCode for CSI Simulation
    * Enable profiling
    * Updated logging and Sample CSI programs
    * Updated the test apps to verify the Direct Mode.
    * Updated to handle socket file removal
    * Added logic to remove private connection on ADVISORY message
    * Implemented logic to save and load the cache at provider side to know the active private connection.
    * Updated memory handling
    * Updated rbuscli for direct connection support
    * Updated the SET/GET Timeout to 15s for RDKB requirement
    
    Signed-off-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
    
    Update access permission of the unix socket (#123)
    
    Updated access permission of the unix for the non-root clients
    
    Update access permission of the unix socket
    
    Updated access permission of the unix for the non-root clients
    
    Fixed the Build Error with CLANG (XE2) and minor fixes (#121)
    
    Fixed the Build Error with CLANG (XE2) and minor fixes
    
    Signed-off-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
    
    Addressed Mutex variable lock & return  (#119)
    
    Addressed Mutex variable lock & return

[33mcommit de7f3113d22edf176a7202477604013c1c0885d0[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Wed Apr 5 17:49:10 2023 -0400

    ARRISXB6-13231 : Observing "CcspWifiSsp" crash with signature rbus as… (#115)
    
    * ARRISXB6-13231 : Observing "CcspWifiSsp" crash with signature rbus async call
    
    Reason for change: Changed rbusAsyncSubscribe_GetSubscription function call
    to have the mutex lock around gRetrier-> items global variable
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>
    
    * ARRISXB6-13231 : Observing "CcspWifiSsp" crash with signature rbus async call
    
    Reason for change: Changed rbusAsyncSubscribe_GetSubscription function call
    to have the mutex lock around gRetrier-> items global variable
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>
    
    ---------
    
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit e9b67de33deab141027297ec7951ef025f9e1631[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Tue Apr 4 23:04:28 2023 +0530

    RDKB-48256: Increase code coverage percentage for rbus_intervalsubscription.c (#116)
    
    Reason for change: Increase code coverage percentage for rbus_intervalsubscription.c
    Test Procedure: run rbusCodeCoverageUI
    Risks: Low
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 0ebed40c7ba646dcae3c4fc89ba41d5185613139[m[33m ([m[1;33mtag: [m[1;33mv2.0.4[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Fri Mar 31 14:39:34 2023 -0400

    Release of rbus-2.0.4
    
    Release of rbus-2.0.4

[33mcommit 323fa847cabcf8a9088687047a607824bb5e564d[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Tue Jan 31 20:56:06 2023 +0530

    Ensure to to have rbusConfig created for all the rbus APIs #99
    
    Reason for change: Requesting sessionid by CcspBaseIf_requestSessionID is resulting in a crash as rbus_open was not called by CCSP.
    Test Procedure: Tested and verified
    Risks: Medium
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit e4cba076be61ef95e6e2e30820a25f92e575098e[m
Author: TonyPickConsultRed <117441544+TonyPickConsultRed@users.noreply.github.com>
Date:   Fri Jan 27 20:23:05 2023 +0000

     Change rbusDateTime_t to use fixed size structures (#29)
    
    * Change rbusDateTime_t to use fixed size structures
    
    rbusDateTime_t relies on the platform int size in the underlying
    structures. This causes faults when mixing lib32 and lib64 rbus
    clients.
    
    This change modifies the underlying definition to use fixed size
    data fields, and adds conversion routines to move that to/from
    the underlying platform 'struct tm' representation.
    
    This also replaces the printf() and scanf() format specifiers with
    PRId64 where necessary to avoid compiler warnings.
    
    * Update unit tests
    
    Initial commit updating time did not update exec_rbus_get_test()
    time handling, causing compile fail.

[33mcommit fa84ede2335cea4737ad10dc7efe43e94402d1d7[m
Author: Deepak M <114924314+dm097@users.noreply.github.com>
Date:   Thu Mar 30 22:23:56 2023 +0530

    Setting componentName of RbusHandle to NULL in rbus_close (#112)
    
    Reason for change: Observing CcspCMAgentSsp crash with signature as " strlen" with Fingerprint:91961838 during Reboot and Stability Test
    
    Test Procedure:
    1.Load the test build in TECHNICOLORCBR device
    2.Connect few clients and perform 20 rounds of reboots.
    3.Isolate the device for stability.
    4.Check Stacktrace for Issue reproduction.
    
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepak_m <deepak_m@comcast.com>

[33mcommit bc360ae0859e1a3d237bea5f5b5d1dd5212563b2[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Tue Mar 28 21:40:10 2023 +0530

    RDKB-46677: Provider Crashed with Active Subscription of event with aliasName is not publishing after restart (#114)
    
    Reason for change: Implement provider side resubscribing of subscriptions of event with aliasName, Instnum and wildcard that are recovered if provider restarts due to crash
    Test Procedure: Tested and verified
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit d3ac40d4f3e352249b60205a89967023e2cf03fb[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Mon Mar 27 21:13:08 2023 +0530

    Observing unexpected logs after firmware upgrade (#113)
    
    Added event name to warring message and removed unwanted words from it.

[33mcommit 6760a2923109a2226e4484f63e0fa3f6e6c392d6[m
Author: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
Date:   Thu Mar 16 15:11:45 2023 -0400

    Update not to trim the traceState & traceParent (#110)
    
    * Update not trim the traceState & traceParent
    
    Updated rbus_setOpenTelemetryContext to *not* to trim the traceState & traceParent

[33mcommit 88ea006f5b74808b3c4ba147ad772380f9e9d9d5[m
Author: Deepak M <114924314+dm097@users.noreply.github.com>
Date:   Thu Mar 2 22:28:30 2023 +0530

    Segmentation fault from Func rbus_discoverObjectElements during unit … (#105)
    
    * Segmentation fault from Func rbus_discoverObjectElements during unit test, code Coverage
    
    Reason for change: Segmentation fault from Func rbus_discoverObjectElements during unit test, code Coverage abd build failre with ENABLE_CODE_COVERAGE=ON
    Test Procedure: run Gtest and Codecoverage on Git Workflow and Destop with Ubuntu 22.04 or above
    Risks: Medium
    
    Signed-off-by: Deepak <deepak_m@comcast.com>

[33mcommit 8e5bb0d75e864cc34e86c84b98f85fd757b7f1c2[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Thu Mar 2 20:52:49 2023 +0530

    RDKB-47568: Update rbusSampleProvider.c to set value for RBUS_DATETIME data type (#107)
    
    Reason for change: Changes made in rbusSampleProvider.c to set the value for rbusDateTime.
    Test Procedure: Execute rtrouted, rbusSampleProvider and run rbuscli
    command as set Device.SampleProvider.AllTypes.DateTimeData datetime 2023-02-28T07:09:50 for rbusDatetime.
    Risks: Medium
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 01a4824e78d1c2c16214d1a099645f62190667e8[m
Author: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
Date:   Mon Feb 27 17:20:47 2023 -0500

    Update rbus.c (#106)

[33mcommit dae29617901ec413f9e5da8fa619cd35fe971404[m
Author: TonyPickConsultRed <117441544+TonyPickConsultRed@users.noreply.github.com>
Date:   Wed Feb 8 19:19:37 2023 +0000

    C make standards (#100)
    
    * Change rbusDateTime_t to use fixed size structures
    
    rbusDateTime_t relies on the platform int size in the underlying
    structures. This causes faults when mixing lib32 and lib64 rbus
    clients.
    
    This change modifies the underlying definition to use fixed size
    data fields, and adds conversion routines to move that to/from
    the underlying platform 'struct tm' representation.
    
    This also replaces the printf() and scanf() format specifiers with
    PRId64 where necessary to avoid compiler warnings.
    
    * Update unit tests
    
    Initial commit updating time did not update exec_rbus_get_test()
    time handling, cauing compile fail.
    
    * Use Better sizes for platform independent time
    
    The original commit uses int64_t for the time struct.
    
    This was clearly overkill given the represented
    information in rbusDateTime_t. This change brings
    these to int32_t, which is also compatible with
    the original printf() and scanf() format specifiers.
    
    * Switch Cmake bin and lib hardcoding
    
    The current MakeLists.txt tend to harcode bin and lib
    destinations.
    
    This change uses the GNU standards module to define
    these locations.
    
    * Make full paths even when GNU paths available

[33mcommit b350d520d73f2f72887c6e8290a3010801fcdf91[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Wed Feb 15 15:23:41 2023 -0500

    TCXB7-5676 - Killing of RBUS Daemon is not rebooting the device (#102)
    
    Reason for change: Changed ExecStop to ExecStopPost. With which device
    is getting rebooted when rtrouted process is killed
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>

[33mcommit 3aa9371c73afa7994963e8c47aae657a2c6d4220[m[33m ([m[1;33mtag: [m[1;33mv2.0.3[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Fri Feb 3 08:34:26 2023 -0500

    Release of rbus-2.0.3
    
    Release of rbus-2.0.3

[33mcommit f2fe37ca16a010124c349332942cce53c7d1f9d8[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Fri Jan 27 22:53:10 2023 +0530

    RDKB-45777: Sample code for rbus_session_mgr (#98)
    
    * RDKB-45777: Sample code for rbus_session_mgr
    
    Reason for change: For testing functionality of rbusSession APIs
    Test Procedure: Execute command ./sample_rbus_session_mgr
    Risks: Low
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    
    * Update sample_rbus_session_mgr.c
    
    * Update sample_rbus_session_mgr.c
    
    * Update CMakeLists.txt
    
    ---------
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit a254cbdb270f502817a740a005e8ef0dee2e7180[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Fri Jan 27 22:16:45 2023 +0530

    RDKB-46735: Addressing Coverity issues (#89)
    
    * RDKB-46735: Addressing Coverity issues
    
    Reason for change: Fixed Coverity leak
    Test Procedure: Tested and verified
    Risks: Low
    Priority: P1
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    
    * Update rbus.c
    
    ---------
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 874979fc410d1fc1a62ad77221e783cd90dfa040[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Fri Jan 27 20:59:56 2023 +0530

    Added duration complete event type while receiving event (#93)
    
    * Added duration complete event type while receiving
    
    Handling duration complete event type while receiving event
    
    * Removing dration completed subscriptionn from provider & consumer side subscriptions records
    
    * Removed unwanted mutex lock

[33mcommit 05e4437a9aefcc1044f91a3fa74fb023e4cddb39[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Fri Jan 27 09:47:00 2023 -0500

    RDKCOM-3564 : Memory Leak when calling rbus_discoverComponentDataElements (#97)
    
    Reason for change: Fixed memory leak with rbus_discoverComponentDataElements,
    rbus_discoverObjectElements and rbus_discoverObjectsElements
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>

[33mcommit 5cfd9ffae2e859fe9836cbb5f1cd941a796d3a31[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Tue Jan 24 16:03:40 2023 -0500

    RDKB-44740 RDKB-44861 : Add Support to RBUS Subscribe API to get Initial Value (#96)
    
    Reason for change: Fixed the crash issue reported with initial value
    subscription
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>

[33mcommit a5f03e9258329ac40b99c427ce998693b72d9d90[m
Author: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
Date:   Tue Jan 24 12:39:46 2023 -0500

    Revert "RDKB-46677: Provider Crashed with Active Subscription of event with aliasName is not publishing after restart (#88)" (#95)
    
    This reverts commit eac05d73716d788bf71a0dae25b81e5bfa1632cd.

[33mcommit ac48aace749f901ff3f8e730d3c5c4191b19e290[m
Author: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
Date:   Mon Jan 23 13:51:22 2023 -0500

    Fix the buffer size to 8K for Camera devices (#91)
    
    Signed-off-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
    
    Signed-off-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit 9ba8cb0c0a4675f1fe3941aa858b35099f44b63b[m
Author: Deepak M <114924314+dm097@users.noreply.github.com>
Date:   Fri Jan 20 20:34:22 2023 +0530

    RDKB-46634 Rbus build warnings from src folder needs fix on Ubuntu-22.10 (#86)
    
    * RDKB-46634 Rbus build warnings from src folder needs fix on Ubuntu-22.10 ver
    
    Reason for change: three Build Warning (-Waddress) observed while compiling RBus files rtError.c rtrouted.c in src/ and rbusTestProvider.c in test/ folder on Ubuntu 22.10
    Test Procedure: Build rbus on Ubuntu 21.10.
    Risks: Med
    Priority: P2
    
    Signed-off-by: Deepak <deepak_m@comcast.com>

[33mcommit 51103163c81c9c60e1234fbb76b5b344999b9899[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Thu Jan 19 21:59:04 2023 +0530

    RDKB-46677: Provider Crashed with Active Subscription of event with aliasName is not publishing after restart (#88)
    
    Reason for change: Implement provider side resubscribing of subscriptions of event with aliasName that are recovered if provider restarts due to crash
    Test Procedure: Tested and verified
    Risks: Medium
    Priority: P1
    
    Change-Id: Iec728f45b2719d29dc1f0d2fc25e5a235d4ccf2d
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 0cf317cead44dc0cbf515c9fef60d12593edac80[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Thu Jan 19 19:55:19 2023 +0530

    RDKB-46554: Memory leak in EventClientAPIs.rbus_subscribeToEventTimeout_test1 (#87)
    
    Reason for change: Fixed memory leak issue
    Test Procedure: Test and verified with valgrind
    Risks: Low
    
    Change-Id: I0108435d3c48d5c1bbc57c0dbc30beab660b399d
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 7b4716fdfc7d706c260e573967cfe7f397fb2f8e[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Wed Jan 18 03:21:05 2023 +0530

    RDKB-45942: Active Subscription is not publishing after Provider crash recovery (#82)
    
    * RDKB-45942: Provider Crashed with Active Subscription is not publishing after restart
    
    Reason for change: Implement provider side resubscribing of subscriptions that are recovered if provider restarts due to crash
    Test Procedure: Tested and verified
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    
    * Update rbus.c
    
    * Fixed the order of subscription and renamed the function
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
    Co-authored-by: Karunakaran A <Karunakaran_Amirthalingam@cable.comcast.com>

[33mcommit ca60788f4409e886bbd9893ef57a4741e635aa7a[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Tue Jan 17 22:11:37 2023 +0530

    RBUS - Support for duration based subscription (#83)

[33mcommit 0cf86c64449fec87a89a841c43becbb9a509876f[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Wed Jan 11 20:23:53 2023 -0500

    RBUS Subscribe API to get Initial Value - CLI update (#81)
    
    * RDKB-44740 RDKB-44861 : RBUS Subscribe API to get Initial Value
    
    Reason for change: Changes done in rbuscli to support initial value
    for subscription with interval and also handled possible invalid
    arguments passed
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR gururaja_erodesriranganramlingham@comcast.com

[33mcommit 32ae5f81436bf8d68bbd574a8a78656934470eed[m
Author: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
Date:   Tue Jan 10 13:16:25 2023 -0500

    Clear the context upon receiving (#80)

[33mcommit 27d5a45c139e69419d4c6b552963c8268536c56b[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Mon Jan 9 18:24:19 2023 -0500

    RDKB-44740 RDKB-44861 : RBUS Subscribe API to get Initial Value (#78)
    
    Reason for change: Added support for passing initial value while
    subscribing in rbuscli. Also extended support to get initial value for
    subscribe with filter and interval
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR gururaja_erodesriranganramlingham@comcast.com

[33mcommit 7a30b7dce782686a067e6cd5df9335f99c8f6768[m
Author: Deepak M <114924314+dm097@users.noreply.github.com>
Date:   Mon Jan 9 20:21:27 2023 +0530

    Random segmentation fault occurring while running rbusTestConsumer app (#79)
    
    Reason for change: Observed Segmentation fault when ran rbusTestConsumer app on Ubuntu 21.10.
    Test Procedure: Build rbus on Ubuntu 21.10, run rbusTestProvider and rbusTestConsumer -a with gdb and bt
    Risks: Med
    Priority: P1
    
    Signed-off-by: Deepak <deepak_m@comcast.com>

[33mcommit ead75ffd64ca8b6cf3afecb885dd79a3102a08cc[m
Author: Deepak M <114924314+dm097@users.noreply.github.com>
Date:   Thu Jan 5 22:04:38 2023 +0530

    RDKC-12670 remove rtrouted.conf (#74)
    
    Reason for change: Add the rtrouted_default.conf to build/patches folder of xcam3 and rtrouted.conf to xcam2 and meta-rdk-comcast-camera receipe by creating rbusdevice and remove rtrouted_default.conf and rtrouted.conf from github federated rbus
    Test Procedure: Trigger RDKC Build and test rtrouted.conf is installed respective products, verify rtrouted exe
    Risks: Med
    Priority: P1
    
    Signed-off-by: Deepak <deepak_m@comcast.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 67dab87c5200ee13fd33b1b30d19101bdba27144[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Wed Jan 4 22:00:36 2023 +0530

    RDKB-45777: [TDK][AUTO][22Q4_SPRINT][CCSPCOMMON_MBUS] Requesting sessionid by CcspBaseIf_requestSessionID is resulting in a crash (#77)
    
    Reason for change: Requesting sessionid by CcspBaseIf_requestSessionID is resulting in a crash
    Test Procedure: Tested and verified
    Risks: Medium
    Priority: P1
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 0abf7a238d7a522fe194e65af86d9ff8b0e9824d[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Tue Jan 3 12:27:56 2023 -0500

    RDKB-46420 RDKB-46368 : Memory leak on Testapp and OneWifi (#76)
    
    * RDKB-44740 RDKB-44861 : RBUS Subscribe API to get Initial Value
    
    Reason for change: Fixed the memory leak issue reported while testing
    with testapp.
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR gururaja_erodesriranganramlingham@comcast.com
    
    * RDKB-46420 RDKB-46368 : Memory leak on Testapp and OneWifi
    
    Reason for change: Fixed the memory leak issue reported while testing with testapp
    This also should fix the OneWifi memory leak as both the valgrind result points
    to the same location
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>

[33mcommit d4f0718be7da56178dc7ec9844260175e149fa6c[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Fri Dec 30 12:12:08 2022 -0500

    RDKB-44935 : [RBUS] Add TraceContext support for distributed tracing (#75)
    
    Reason for change: Fixed memory leak issue
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 512f4fa19ab83c804759869499b9abe9b52d31e0[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Fri Dec 30 08:35:55 2022 +0530

    RBUS - Support for interval-based subscriptions (#70)
    
    * RBUS - Support for interval-based subscriptions
    * Added ERROR log
    * rbuscli changes

[33mcommit 7aa1c7f1b0865ad7520cda7aaaad8f559fce8239[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Thu Dec 29 07:49:28 2022 -0500

    RDKB-44740 RDKB-44861 : RBUS Subscribe API to get Initial Value (#73)
    
    Reason for change: Fixed the gtest issue reported by tester.
    Also added new test cases in rbusEventConsumer and rbusTableConsumer
    to test the subscribe with initial value
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR gururaja_erodesriranganramlingham@comcast.com

[33mcommit a2706b819a97c37208781c83dc1363a771e50f15[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Wed Dec 21 08:18:07 2022 -0500

    RDKB-45907: The routing logic change rbus is redefined (#71)
    
    Reason for change: Revert "RDKB-45652: Can't find destination component
    for multiple parameters (rdkcentral#39)"
    This reverts commit 6950243
    Revert "if else alignment change for better readability (rdkcentral#34)"
    This reverts commit 449f1c7
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>

[33mcommit 5234af0cb05d690045a4e712b4e54fc4dee90773[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Mon Dec 19 15:41:11 2022 -0500

    RDKB-46238 : rbusMethodConsumer testapp crash (#65)
    
    Reason for change: Single outParams variable is used in the rbusMethodConsumer.
    Whenever rbusMethod_Invoke() is called with a method which doesn't exist
    in the provider side, then the outParam value is NULL.
    Which is getting dereferenced in rbusObject_fwrite() resulted in crash.
    Handled outParam in rbus when the rbuscore returns failure.
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>

[33mcommit 9f11132564b0dd1719eb5cbebd3122c070c3dee4[m
Author: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
Date:   Fri Dec 16 09:33:48 2022 -0500

    Revert "Revert "RDKB-44740 RDKB-44861 : RBUS Subscribe API to get Initial Value (#62)" (#63)" (#64)
    
    This reverts commit 43bdf8b0348dd023cc3bbb25a53911f6e1e78dff.

[33mcommit 4c185cb17b3eea24e74882c4951f349fcf91e48a[m
Author: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
Date:   Thu Dec 15 23:57:03 2022 -0500

    Revert "RDKB-44740 RDKB-44861 : RBUS Subscribe API to get Initial Value (#62)" (#63)
    
    This reverts commit 518341ff2235974227288c24449e61883dab1006.

[33mcommit 2b14904fc4ca341441cc227ac9a4c2aa87b99c0a[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Thu Dec 15 23:42:31 2022 -0500

    RDKB-44740 RDKB-44861 : RBUS Subscribe API to get Initial Value (#62)
    
    Reason for change: Whenever subscribeEx is called with PublishOnSubscribe
    set(new element added to rbusEventSubscription_t structure), we will publish
    the initial value of the data model as soon as the subscription is sent.
    The data is sent in the below format
    For property/event:
    {"initialValue": <data model value at time of subscription>}
    Ex: {"initialValue": 123}
    For Table:
    {"numberOfEntries": <row count>, "path1": <path1 name>, "path2": <path2 name>, "path3":<path3 name>}
    Ex: {"numberOfEntries": 3, "path1": "a.1", "path2": "a.2", "path3":"a.3"}
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR gururaja_erodesriranganramlingham@comcast.com

[33mcommit 6a7f7cce3fa151cf96e6f301e1907698fb9bfa14[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Fri Dec 9 03:49:37 2022 +0530

    RBUS cli utility doesn't have the provision to given the interval as argument (#59)
    
    * RDKB-45878: RBUS cli utility doesn't have the provision to give the interval as argument
    
        Reason for change: Added interval based subscription
        Test Procedure: execute command "subint <event> <interval> in rbuscli
        Risks: Low
        Priority: P0
        Signed-off-by: Netaji Panigrahi <Netaji_Panigrahi@comcast.com>

[33mcommit d8fc116c713dc1dd87a00644288793ec99b8ad7d[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Wed Dec 7 03:00:24 2022 +0530

    RDKB-45774:[TDK][AUTO][22Q4_SPRINT][CCSPCOMMON_MBUS]CcspBaseIf_informEndOfSession API returns failure with status 102 (#56)
    
    Reason for change: CcspBaseIf_informEndOfSession API returns failure with status 102
    Test Procedure: CcspBaseIf_informEndOfSession API should return success
    Risks: Low
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit a1d8f8d60a06a3a7124957dc967241797bbf216a[m[33m ([m[1;33mtag: [m[1;33mv2.0.2.1[m[33m)[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Mon Jan 23 16:48:50 2023 -0500

    Fix the buffer size to 8K for Camera devices (#92)
    
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
    Co-authored-by: Karunakaran A <Karunakaran_Amirthalingam@cable.comcast.com>

[33mcommit 2a7f8eacf1f33e291fa1ade3bc83978b2e103892[m[33m ([m[1;33mtag: [m[1;33mv2.0.2[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Wed Nov 30 15:02:04 2022 -0500

    Release of RBUS v2.0.2
    
    Signed-off-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit 522e8e515d95b8684bc2cd5028ecd46e70aa2987[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Wed Nov 30 20:49:43 2022 +0530

    RDKB-44725: rbuscli set failing when commit field is passed (#54)
    
    Reason for change: rbuscli set failing when commit field is passed.
    Test Procedure: execute command set Device.X_RDK_WanManager.CPEInterface.1.Wan.Name string wan0 false in rbuscli
    Risks: Low
    
    Signed-off-by: Deepthi DEEPTHICHANDRASHEKAR_SHETTY@comcast.com

[33mcommit 1a9c6d597d83721deaee66db7a29cfb59a244e26[m
Author: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
Date:   Tue Nov 29 16:20:41 2022 -0500

    Add unit test to ensure better testing and code coverage (#53)
    
    RDKB-45123: Add unit test to ensure better testing and code coverage

[33mcommit 44f200efe52212a67242d0306818cf78d6a129f7[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Wed Nov 30 00:14:53 2022 +0530

    RDKB-45052: Increase Code coverage percentage for rtmessage (#50)
    
    * RDKB-45052:Increase Code coverage percentage for rtmessage
    
    Reason for change: Increase Code coverage percentage for rtmessage
    Test Procedure: Run rbusCodeCoverageUI
    Risks: Low
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
    Co-authored-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 67aba27ac4418c46fbf592cf412ca3c3f20097de[m
Author: Deepak M <114924314+dm097@users.noreply.github.com>
Date:   Sun Nov 20 04:00:56 2022 +0530

    RDKB-45280:Update the rtLog to support RDKLogger appropriately and modify rbus and rbuscore to use the rtLog Macro (#35)
    
    * *RDKB-45280:Update the rtLog to support RDKLogger appropriately and modify rbus and rbuscore to use the rtLog Macro
    
    Reason for change:Update the rtLog to support RDKLogger appropriately and modify rbus and rbuscore to use the rtLog Macro
    Test Procedure: Tested on VM: Run-rtrouted for RTMESSAGE log (Terminal1), RbusProvider(Term2) and rbuscli or rbusConsumer For RBUS logs(Term3), rbus_session_mgr For RBUSCORE logs(Term4)
    Risks: Low
    Priority: P1
    
    Signed-off-by: Deepak <deepak_m@comcast.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
    
    RDKC-12563,RDKB-45841 - Continuous flooding seen in various logs from RBUS module (#52)
    
    * RDKC-12563: Continuous flooding seen in various logs from RTMESSAGE and RBUS module
    * RDKB-45841 : Log files are empty after 5 min uptime after upgrade/reboot/FR
    
    Reason for change: Change for RDK logger for we used the rtLogLevel_t as is to pass to RDKLogger for rdk_LogLevel which is wrong, RT_LOG_DEBUG is 0 where as RDK_LOG_FATAL is also 0. So this lead all the debug to be printed as FATAL and similarly, all the INFO to be printed as ERROR. Test Procedure: Trigger RDKC and RDKB Build and test by flashing respective images and verify Logs of on Device. Risks: Med
    Priority: P0

[33mcommit 01e7f72022916e52146d9eb4ea5e73f0d28176d3[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Fri Nov 18 21:26:13 2022 +0530

    Added libexchanger dependency for rbus (#40)
    
    * Update CMakeLists.txt & rdk_env.xml
    
    Added libsyswrapper source path
    Added libexchanger dependency for rbus
    
    Set 'RDKC_BUILD=ON' for configuration (#41)
    
    * Update rdk_build.sh
    
    Set 'RDKC_BUILD=ON' for configuration
    
    Update rdk_build.sh (#43)
    
    Copied rtrouted config file based on component
    
    *RDKC-12575:[CI][AUTO] Xw4 Version retrieved are different after CDL - RDKLOGGER is not ON in xCAM2 and xCAM3 (#44)
    
    Reason for change: Enabled ENABLE_RDKLOGGER=ON in rdk_build.sh
    Test Procedure: Trigger RDKC Build and test, tested on Local builld PC, Test on jenkins
    Risks: Med
    Priority: P0
    
    Signed-off-by: Deepak <deepak_m@comcast.com>
    
    Added breakpadwrap dependency and enabled -DINCLUDE_BREAKPAD=ON (#42)
    
    * Added breakpadwrap dependency and enabled -DINCLUDE_BREAKPAD=ON
    for Xcam3 and xcam2 but NOT for XWRUS(XW)
    
    * Added breakpadwrap dependency for Xcam3 and xcam2 but NOT for XWRUS(XW)
    
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
    
    XWRBUS failed on CONFIGURE (#45)
    
    * Update rdk_env.xml
    
    Added 'xwrdklogger' as a dependency
    
    Fixing the XW4 build failure with RDKLogger dependency (#47)
    
    Fixing the XW4 build failure with RDKLogger dependency
    
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
    
    Enable RDKLogger and Disable Breakpadwrap Support (#48)
    
    * Fixed the FSROOT PATH for RDKC & Disable the breakpad support for RDKC for now
    
    Signed-off-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
    
    Enable breakpadwrap Support (#49)
    
    * Enable Breakpadwrap Support

[33mcommit 2e2ff329bc3edd8d45845edc77f2d0b5f1e019c0[m
Author: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
Date:   Thu Nov 17 21:16:46 2022 +0530

    RDKB-42689:RBUS - add a comment in the code to explain Subscribe with autoPublish True
    
    Reason for change: added a comment in the code to explain Subscribe with autoPublish True
    Risks: Low
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 4b90d39167e4a52bac0f69186e93afedaf7fc5f3[m
Author: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>
Date:   Wed Nov 16 21:00:24 2022 +0530

    RDKB-45530: Memory leak in rbus_gtest.bin
    
    Reason for change: Memory leak in rbus_gtest.bin
    Test Procedure: run rbus_gtest.bin with valgrind
    Risks: Low
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 7d1b27db70313a937325eae0b8905ba4ae34c6b1[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Tue Nov 15 21:11:25 2022 -0500

    RDKB-45652: Can't find destination component for multiple parameters (#39) & (#34)
    
    Reason for change: Made changes to discover all the components owner
    for the wild card query and send notification only to the data model
    registered component in case of addition and deletion of property/table
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P0
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>

[33mcommit 6b2490e2311be0a264043e9b48b18428776e21af[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Thu Nov 17 14:47:31 2022 -0500

    Removed cjson include as it is part of FindcJSON.cmake (#38)

[33mcommit 53e0160b00b46a4b4b228d4f418aafc23177a0a3[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Fri Nov 18 00:06:20 2022 +0530

    RDKC - Use Federated RBUS from github (#37)
    
    * RDKC-12438: Migrate XCam2 and Xcam3 Platforms to use RBUS from github
    
    Reason for change: Migrate XCam2 and Xcam3 Platforms to use RBUS from github
    Test Procedure: build test
    Risks: med
    
    Signed-off-by: Deepak <deepak_m@comcast.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>
    Co-authored-by: Netaji Panigrahi <Netaji_Panigrahi@comcast.com>

[33mcommit 0c81bce0d4080e592a68d9859fa1b289e9c95ba0[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Tue Nov 15 21:37:55 2022 +0530

    RDKB-45363:RBUS Sample App (rbusMethodConsumer) crashing (#33)
    
    Reason for change: rbusMethodConsumer and rbusMethodProvider crashing
    Test Procedure: Run rbusMethodProvider and run rbusMethodConsumer with
    valgrind
    Risks: Low
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit dc3dbb00557a72f4aaf17f55641846268551b06d[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Fri Nov 11 20:51:18 2022 +0530

    RDKB-45051:Increase Code coverage percentage for rbuscore (#26)
    
    Reason for change: Increase Code coverage percentage for rbuscore
    Test Procedure: Run rbusCodeCoverageUI
    Risks: Low
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 4a82b5e3853b032aa5c2b5ca0885f3876af0af8e[m
Author: Jacob Gladish <jacobgladish@yahoo.com>
Date:   Wed Nov 9 10:15:18 2022 -0500

    Added basic support for OpenTelemetry context sharing (#24)

[33mcommit 1c129d235607f3767c6e97cade59e9e133e6ac69[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Wed Nov 9 10:07:37 2022 -0500

    RDKC-12448 - cmake find breakpad package (#31)
    
    * RDKC-12448 - cmake find breakpad package
    
    Reason for change: Added support to find the breakpad package using find
    cmake
    Test Procedure: Test and verified
    Risks: low
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>

[33mcommit 86452533b1c97852f298ff76b9327e403ebc104c[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Fri Nov 4 19:05:51 2022 +0530

    RDKB-45363:RBUS Sample App (rbusMethodConsumer) crashing (#30)
    
    * RDKB-45363:RBUS Sample App (rbusMethodConsumer) crashing
    
    Reason for change: rbusMethodConsumer crashing
    Test Procedure: Run rbusMethodProvider and run rbusMethodConsumer with
    valgrind
    Risks: Low
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit f11acc189c813215e53e30380092b5d0f3d00c0f[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Fri Oct 28 00:14:26 2022 +0530

    RDKB-45050:Increase Code coverage percentage for RBUS (#17)
    
    Reason for change: Increase Code coverage percentage for RBUS
    Test Procedure: Run rbusCodeCoverageUI
    Risks: Low
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 341ae426aa9d50a386814219defac2007c6b17df[m[33m ([m[1;33mtag: [m[1;33mv2.0.1[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Tue Nov 1 12:30:25 2022 -0400

    Release of RBUS v2.0.1
    
    Release of RBUS v2.0.1

[33mcommit f8eeb07730af898a9c4fd5696ba1a7265b24308a[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Tue Nov 1 11:00:54 2022 -0400

    Changed the read buffer size from 8 to 64 KB (#25)

[33mcommit 80db370cc3d28e6c12bf56e411d11be078537aff[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Thu Oct 27 16:57:03 2022 -0400

    Changes for doorbell camera build issue with federated rbus (#21)

[33mcommit 18b919baecc34a963db1019c5b0c7098e07a0fa2[m
Author: Deepak M <114924314+dm097@users.noreply.github.com>
Date:   Fri Oct 21 22:35:45 2022 +0530

    Free the memory leaks definitely lost from rbus gtest (#19)
    
    * Free the memory leaks definitely lost from rbus gtest
    
    Reason for change: rectified incorrect initialization of structure rbusDateTime_t
    Test Procedure: Build rtmessage, rbuscore and start rtrouted", Build Rbus Gtests with valgrind mentioned in Jira Description
    Risks: Low
    Priority: P2
    
    Signed-off-by: Deepak <deepak_m@comcast.com>

[33mcommit bfbbf4a6e01497079351d67af0d57f03809d067a[m
Author: Deepak M <114924314+dm097@users.noreply.github.com>
Date:   Fri Oct 21 19:20:33 2022 +0530

    RDKB-44613: RDKB - Use RBUS from RDKCentral github (#18)
    
    Reason for change: Updating Karuna's change in rtmessage CMakelists.txt, and other changes for solving some Rbus dependency errors for bitbake comcast-broadband-dev-image.
    Test Procedure: making changes on TECHNICOLORXB7 and checking "bitbake f -C compile rbus" an check is rbus is downloaded to work/ directory and all required lib and include files are installed (right now with INSANE_SKIP). The test full build "bitbake comcast-broadband-dev-image" and resolving component dependencies with Rbus
    Risks: Low
    Priority: P1
    
    Change-Id: I23af02926c84eb9b1aa8400deb2f7161738fa00f
    Signed-off-by: Deepak <deepak_m@comcast.com>
    
    Signed-off-by: Deepak <deepak_m@comcast.com>

[33mcommit 1910b0ee447d334533195a19f7f68454348b8bca[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Thu Oct 20 20:09:32 2022 +0530

    RDKB-44856 :Combine rbus and rbuscore Unittest Programs under one single binary (#15)
    
    Reason for change: While executing rbus_gtest.bin, Getting core_dump error
    Test Procedure: Run rbus_gtest.bin to execute both core and rbus unittests
    Risks: Low
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 8d7d44fa8f2c4ab096f72420484d1fe69c700670[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Thu Oct 20 21:10:56 2022 +0530

    workflow for unit tests (#14)
    
    Running rbusTestProvider, rbusTestConsumer for unit tests.

[33mcommit c7d3198985bb989ffc9b3bb37e75226dddc38d43[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Mon Oct 17 16:33:36 2022 -0400

    RBUS Publish wrongly prints RTT time as 0 (#13)
    
    Reason for change: Changes to fix the telemetry crash issue faced
    with RTT 0 sec issue fix
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>
    
    RBUS Publish wrongly prints RTT time as 0 (#13)
    
    * RDKB-44765 - Subscribe and Publish leaves RTT time as 0
    
    Reason for change: rbusEvent_Publish() function doesn't wait for
    response message, So whenever timeout or consumer exit during publish call
    it might lead to print of timeout value which is not proper. Added
    condition check to print the timeout value only in case of response and
    request mesaage between consumer and provider.
    Test Procedure: Test and verified
    Risks: Medium
    Priority: P1
    Signed-off-by: Gururaaja ESR <gururaja_erodesriranganramlingham@comcast.com>
    Co-Authored-By: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit 8ee3f9c4207808f347a01416b403b3476c50f247[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Mon Oct 17 20:01:30 2022 +0530

    Handle the order of adding node to the routing tree (#16)
    
    RDKB-44839: Order of Table and its child registration in RBUS is addressed  (#12)
    
    Reason for change: Facing Duplicate entry while a registring table with "Foo.SomeTable.{i}." format.
                         As per condition get routeList2 if the requested topic ends with "."
                         and it is not matched with routeList2 topics.
      Solution: Removed condition check and returned route list instead of routeList2.
      Test Procedure: 1. Run test using rbusTestProvider, rbusTestConsumer
                      2. check Onewifi CSI feature broken issue.
      Risks: Low
      Signed-off-by: Netaji <NETAJI_PANIGRAHI@comcast.com>
    
    Co-Authored-By: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit 97472cd46d346005b56b5d282bfe55d48b93bc29[m
Author: Netaji Panigrahi <114923459+NetajiPanigrahi@users.noreply.github.com>
Date:   Sat Oct 15 02:09:06 2022 +0530

    Workflow to run the build for every check-in (#11)
    
    * workflow to run the build for every check-in
    
    GitHub action for build
    Build workflow should run for future rbus branches like 2.1, 2.2, 3.0 etc.

[33mcommit fef3e8d43b57e65ad4f8c8eddfeaaba0ce2bad10[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Fri Oct 14 13:21:23 2022 -0400

    Included session manager to fix gtest unit test failure (#10)
    
    * Included session manager to fix gtest unit test failure

[33mcommit 13279a15f13f18477c1348eaf31f58ada1b694a5[m
Author: gururaajar <83449026+gururaajar@users.noreply.github.com>
Date:   Fri Oct 14 12:02:55 2022 -0400

    Install header files in given PREFIX path (#8)
    
    RDKB-44953 : Install header files in given PREFIX path
    
    Co-authored-by: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>

[33mcommit 30195d9e16fef86538dd4c7e441febdbed9bffc4[m
Author: Deepthi C Shetty <115452109+dshett549@users.noreply.github.com>
Date:   Fri Oct 14 02:41:01 2022 +0530

    RDKB-44884 : Enable Code Coverage on the Federated RBUS Component (#6)
    
    Reason for change: Enable Code Coverage on the Federated RBUS Component
    Test Procedure: Execute gtest and code coverage
    Risks: Low
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit a91036204b7d9851bbf8704a451bb9e3a65121a0[m
Author: dshett549 <115452109+dshett549@users.noreply.github.com>
Date:   Wed Oct 12 00:41:20 2022 +0530

    Combine rbus and rbuscore Unittest Programs under one single binary (#5)
    
    * RDKB-44856 : Combine rbus and rbuscore Unittest Programs under one single binary
    
    Reason for change: Combine rbus and rbuscore Unittest Programs under one single binary
    Test Procedure: Run rbus_gtest.bin to execute both core and rbus unittests
    Risks: Low
    
    Signed-off-by: Deepthi <DEEPTHICHANDRASHEKAR_SHETTY@comcast.com>

[33mcommit 4746c07ddffd9d7f38133cfa823ccf53f93b6b51[m
Author: Deepak M <114924314+dm097@users.noreply.github.com>
Date:   Tue Oct 11 06:37:52 2022 +0530

    Free the memory leaks from rbus gtest (#2)
    
    Free the memory leaks from rbus gtest
    
    Reason for change: rectified incorrect initialization of structure rbusDateTime_t
    Test Procedure: Build rtmessage, rbuscore and start rtrouted", Build Rbus Gtests with valgrind mentioned in Jira Description
    Risks: Low
    Priority: P2
    
    Change-Id: Iff54f92e2f8b555a40b78d2a431b4cf9d033fb3c
    Signed-off-by: Deepak <deepak_m@comcast.com>
    
    Signed-off-by: Deepak <deepak_m@comcast.com>
    Co-authored-by: Karunakaran A <48997923+karuna2git@users.noreply.github.com>

[33mcommit 7a35d1a5afbfe21682f0a23f7b7f8c11bb6d34af[m
Author: Netaji Panigrahi <Netaji_Panigrahi@comcast.com>
Date:   Mon Oct 3 18:51:34 2022 +0530

    Fix the memory Leaks in rbus gtest programs
    
    Reason for change: Added null check in rbusMessage_Release function
        Test Procedure: Build and test rbus, run valgrind
        Risks: Low
        Priority: P2

[33mcommit a9ea04215d58b4cc59d523cbd2b95a65cc1deb49[m[33m ([m[1;33mtag: [m[1;33mv2.0.0[m[33m)[m
Author: Karunakaran A <karunakaran_amirthalingam@cable.comcast.com>
Date:   Tue Sep 20 15:27:43 2022 -0400

    Initial Commit
    
    Initial Commit
